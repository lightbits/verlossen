#include <stdio.h>
#include "SDL.h"
#include "net.cpp"
#include "platform.cpp"
#include "test_game.cpp"

struct Client
{
    NetAddress address;
    PlayerNum  player_index;
    Sequence   last_ack;
    GameInput  last_input;
    uint64     last_recv_time;
    uint64     last_send_time;
    int        rate;
    bool       connected;
};

struct ClientList
{
    Client entries[MAX_PLAYER_COUNT];
    int count;

    void
    Remove(int index)
    {
        entries[index].connected = false;
        count--;
    }

    Client *
    Add(NetAddress &address)
    {
        if (count >= MAX_PLAYER_COUNT)
            return 0;
        // Find first available slot
        int slot = 0;
        while (slot < MAX_PLAYER_COUNT &&
               entries[slot].connected) {
            slot++;
        }
        Assert(slot < MAX_PLAYER_COUNT);
        Client c = {};
        c.address = address;
        c.connected = true;
        entries[slot] = c;
        count++;
        return &entries[slot];
    }

    Client *
    Find(NetAddress &address)
    {
        uint32 b_ip = address.ip_bytes;
        uint16 b_port = address.port;
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            uint32 a_ip = entries[i].address.ip_bytes;
            uint16 a_port = entries[i].address.port;
            if (a_ip == b_ip && a_port == b_port)
                return &entries[i];
        }
        return 0;
    }
};

struct App
{
    bool running;
};

struct Network
{
    ClientList clients;
    Sequence   sequence; // Tags the next snapshot we send to clients
};

static Network net;
static App app;

void
SendAccept(NetAddress &to)
{
    ServerUpdate p = {};
    p.protocol = SV_ACCEPT;
    p.sequence = net.sequence;
    NetSend(to, p);
}

void
SendReject(NetAddress &to)
{
    ServerUpdate p = {};
    p.protocol = SV_REJECT;
    p.sequence = net.sequence;
    NetSend(to, p);
}

void
SendUpdate(NetAddress &to, GameState &state)
{
    ServerUpdate p = {};
    p.protocol = SV_UPDATE;
    p.state = state;
    p.sequence = net.sequence;
    NetSend(to, p);
}

void
PollNetwork(GameState &state)
{
    NetAddress sender = {};
    ClientCmd incoming = {};
    while (NetRead(incoming, sender))
    {
        switch (incoming.protocol)
        {
        case CL_CONNECT:
        {
            Client *c = net.clients.Add(sender);
            if (c)
            {
                c->player_index = GameAddPlayer(state);
                c->last_ack = incoming.expected - 1;
                c->last_recv_time = GetTick();
                c->rate = incoming.rate;
                SendAccept(sender);
            }
            else
            {
                SendReject(sender);
            }
        } break;
        case CL_UPDATE:
        {
            Client *c = net.clients.Find(sender);
            if (c)
            {
                c->last_input = incoming.input;
                c->last_ack = incoming.expected - 1;
                c->rate = incoming.rate;
                c->last_recv_time = GetTick();
            }
            else
            {
                printf("Received update from dropped client\n");
            }
        } break;
        }
    }
}

void
PrintDebugStuff(GameState state)
{
    NetStats stats = NetGetStats();
    printf("sequence: %d\n", net.sequence);
    printf("players: %d\n", state.player_count);
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        Client *c = &net.clients.entries[i];
        NetAddress address = c->address;
        PlayerNum  player_index = c->player_index;
        Sequence   last_ack = c->last_ack;
        GameInput  last_input = c->last_input;
        uint64     last_recv_time = c->last_recv_time;
        uint64     last_send_time = c->last_send_time;
        int        rate = c->rate;
        bool       connected = c->connected;
        if (connected)
        {
            printf("%d ", player_index);
            printf("%d.%d.%d.%d:%d ",
                   address.ip0, address.ip1,
                   address.ip2, address.ip3,
                   address.port);
            printf("%d %d\n",
                   state.players[player_index].x,
                   state.players[player_index].y);
        }
    }
    printf("%.2f KBps out", stats.avg_bytes_sent / 1024);
    printf("\t%.2f KBps in\n", stats.avg_bytes_read / 1024);
    printf("\n");
}

int
main(int argc, char **argv)
{
    int listen_port = 12345;
    if (argc == 2)
    {
        sscanf(argv[1], "%d", &listen_port);
    }
    NetSetPreferredListenPort(listen_port);

    GameState state = {};
    GameInit(state);

    uint64 initial_tick = GetTick();
    uint64 last_game_tick = initial_tick;
    int tickrate = 20;
    float tick_interval = 1.0f / float(tickrate);
    float client_timeout_interval = 2.0f;
    app.running = true;
    net.sequence = 0;
    while (app.running)
    {
        uint64 tick = GetTick();
        PollNetwork(state);

        if (TimeSince(last_game_tick) > tick_interval)
        {
            GameInput inputs[MAX_PLAYER_COUNT];
            for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            {
                Client *c = &net.clients.entries[i];
                if (!c->connected)
                    continue;
                int input_index = c->player_index;
                inputs[input_index] = c->last_input;
            }
            GameTick(state, inputs, tick_interval);
            last_game_tick = tick;
            net.sequence++;

            PrintDebugStuff(state);
        }

        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            Client *c = &net.clients.entries[i];
            if (!c->connected)
                continue;

            int rate = c->rate == 0 ? 20 : c->rate;
            if (TimeSince(c->last_send_time) >
                1.0f / float(rate))
            {
                c->last_send_time = GetTick();
                SendUpdate(net.clients.entries[i].address, state);
            }

            if (TimeSince(c->last_recv_time) >
                client_timeout_interval)
            {
                printf("Client %d.%d.%d.%d timed out.\n",
                       c->address.ip0, c->address.ip1,
                       c->address.ip2, c->address.ip3);
                GameDropPlayer(state, c->player_index);
                net.clients.Remove(i);
            }
        }
    }

    return 0;
}
