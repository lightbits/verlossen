#include <stdio.h>
#include "SDL.h"
#include "net.cpp"
#include "platform.cpp"
#include "test_game.cpp"

struct Client
{
    NetAddress address;
    GameInput input;
    uint64 last_recv_time;
    uint64 last_send_time;
    int rate;
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
    ClientList clients;
};

static App app;

void
SendAccept(NetAddress &to)
{
    ServerPacket p = {};
    p.protocol = SV_ACCEPT;
    NetSend(&to, (const char*)&p, sizeof(p));
}

void
SendReject(NetAddress &to)
{
    ServerPacket p = {};
    p.protocol = SV_REJECT;
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
            Client *c = app.clients.Add(sender);
            if (c)
            {
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
            Client *c = app.clients.Get(sender);
            if (c)
            {
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
    int tickrate = 66;
    int updates_sent = 0;
    float tick_interval = 1.0f / float(tickrate);
    float client_timeout_interval = 2.0f;
    while (1)
    {
        uint64 tick = GetTick();
        PollNetwork();

        if (TimeSince(last_game_tick) > tick_interval)
        {
            GameInput inputs[MAX_CONNECTIONS];
            for (int i = 0; i < app.clients.count; i++)
                inputs[i] = app.clients.entries[i].input;
            GameTick(state, inputs, app.clients.count);
            last_game_tick = tick;
        }

        for (int i = 0; i < app.clients.count; i++)
        {
            Client *c = &app.clients.entries[i];
            int rate = c->rate == 0 ? 20 : c->rate;
            if (TimeSince(c->last_send_time) >
                1.0f / float(rate))
            {
                updates_sent++;
                c->last_send_time = GetTick();
                SendUpdate(state, app.clients.entries[i].address);
            }

            if (TimeSince(c->last_recv_time) >
                client_timeout_interval)
            {
                printf("Client %d.%d.%d.%d timed out.\n",
                       c->address.ip0, c->address.ip1,
                       c->address.ip2, c->address.ip3);
                i = app.clients.Remove(c);
            }
        }

        NetStats stats = NetGetStats();
        printf("\r%d clients\tx = %d\ty = %d\tavg: %.2fKBps out, %.2fKBps in)",
                app.clients.count,
                state.x, state.y,
                stats.avg_bytes_sent / 1024,
                stats.avg_bytes_read / 1024);
    }
}
