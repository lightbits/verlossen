#include <stdio.h>
#include "SDL.h"
#include "net.cpp"
#include "platform.cpp"
#include "test_game.cpp"

struct Client
{
    NetAddress address;
    GameInput  input;
    uint64     last_recv_time;
    uint64     last_send_time;
    Sequence   last_ack;
    int        rate;
};

#define MAX_CONNECTIONS 4
struct ClientList
{
    Client entries[MAX_CONNECTIONS];
    int count;

    int
    Remove(Client *client)
    {
        int i = 0;
        while (i < count)
        {
            if (&entries[i] == client)
                break;
            i++;
        }
        if (i < count)
        {
            for (int j = i; j < count - 1; j++)
                entries[j] = entries[j + 1];
            count--;
        }
        return i;
    }

    Client *
    Add(NetAddress &address)
    {
        if (count >= MAX_CONNECTIONS)
            return 0;
        Client *result = &entries[count];
        result->address = address;
        result->input = GameInput();
        count++;
        return result;
    }

    Client *
    Get(NetAddress &address)
    {
        uint32 b_ip = address.ip_bytes;
        uint16 b_port = address.port;
        for (int i = 0; i < count; i++)
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
    ServerPacket p = {};
    p.protocol = SV_ACCEPT;
    p.sequence = net.sequence;
    NetSend(&to, (const char*)&p, sizeof(p));
}

void
SendReject(NetAddress &to)
{
    ServerPacket p = {};
    p.protocol = SV_REJECT;
    p.sequence = net.sequence;
    NetSend(&to, (const char*)&p, sizeof(p));
}

void
SendUpdate(GameState &state, NetAddress &to)
{
    /* TODO: Don't send the entire state
    */

    ServerPacket p = {};
    p.protocol = SV_UPDATE;
    p.state = state;
    p.sequence = net.sequence;
    NetSend(&to, (const char*)&p, sizeof(p));
}

void
PollNetwork()
{
    NetAddress sender = {};
    ClientPacket incoming = {};
    char *buffer = (char*)&incoming;
    int max_size = sizeof(ClientPacket);
    int read_bytes = NetRead(buffer, max_size, &sender);
    while (read_bytes > 0)
    {
        switch (incoming.protocol)
        {
        case CL_CONNECT:
        {
            Client *c = net.clients.Add(sender);
            if (c)
            {
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
            Client *c = net.clients.Get(sender);
            if (c)
            {
                c->last_ack = incoming.expected - 1;
                c->input = incoming.input;
                c->rate = incoming.rate;
                c->last_recv_time = GetTick();
            }
            else
            {
                printf("Received update from dropped client\n");
            }
        } break;
        }
        read_bytes = NetRead(buffer, max_size, &sender);
    }
}

void
PrintDebugStuff(GameState state)
{
    NetStats stats = NetGetStats();
    printf("\nx = %d y = %d", state.x, state.y);
    printf("\tavg %.2f KBps out", stats.avg_bytes_sent / 1024);
    printf("\t%.2f KBps in", stats.avg_bytes_read / 1024);
    printf("\tsequence %d", net.sequence);
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
    InitGameState(state);

    uint64 initial_tick = GetTick();
    uint64 last_game_tick = initial_tick;
    int tickrate = 20;
    int updates_sent = 0;
    float tick_interval = 1.0f / float(tickrate);
    float client_timeout_interval = 2.0f;
    app.running = true;
    net.sequence = 0;
    while (app.running)
    {
        uint64 tick = GetTick();
        PollNetwork();

        if (TimeSince(last_game_tick) > tick_interval)
        {
            GameInput inputs[MAX_CONNECTIONS];
            for (int i = 0; i < net.clients.count; i++)
                inputs[i] = net.clients.entries[i].input;
            GameTick(state, inputs, net.clients.count);
            last_game_tick = tick;
            net.sequence++;

            PrintDebugStuff(state);
        }

        for (int i = 0; i < net.clients.count; i++)
        {
            Client *c = &net.clients.entries[i];
            int rate = c->rate == 0 ? 20 : c->rate;
            if (TimeSince(c->last_send_time) >
                1.0f / float(rate))
            {
                updates_sent++;
                c->last_send_time = GetTick();
                SendUpdate(state, net.clients.entries[i].address);
            }

            if (TimeSince(c->last_recv_time) >
                client_timeout_interval)
            {
                printf("Client %d.%d.%d.%d timed out.\n",
                       c->address.ip0, c->address.ip1,
                       c->address.ip2, c->address.ip3);
                i = net.clients.Remove(c);
            }
        }
    }

    return 0;
}
