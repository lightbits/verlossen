#include <stdio.h>
#include "SDL.h"
#include "net.cpp"
#include "platform.cpp"
#include "test_game.cpp"

struct Client
{
    NetAddress *address;
    GameInput *input;
};

#define MAX_CONNECTIONS 4
struct ClientList
{
    Client     entries[MAX_CONNECTIONS];
    NetAddress addresses[MAX_CONNECTIONS];
    GameInput  inputs[MAX_CONNECTIONS];
    int        count;
    /*
    TODO: Keep rate and last update sent variables
    to throttle the rate at which we send updates to
    a specific client.

    TODO: Keep last update recv to determine timeouts
    */

    bool
    Add(NetAddress &address)
    {
        if (count >= MAX_CONNECTIONS)
            return false;
        addresses[count] = address;
        inputs[count] = GameInput();
        entries[count].address = &addresses[count];
        entries[count].input = &inputs[count];
        count++;
        return true;
    }

    Client *
    Get(NetAddress &address)
    {
        uint32 b_ip = address.ip_bytes;
        uint16 b_port = address.port;
        for (int i = 0; i < count; i++)
        {
            uint32 a_ip = addresses[i].ip_bytes;
            uint16 a_port = addresses[i].port;
            if (a_ip == b_ip && a_port == b_port)
            {
                return &entries[i];
            }
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
            if (app.clients.Add(sender))
                SendAccept(sender);
            else
                SendReject(sender);
            break;
        case CL_UPDATE:
            Client *c = app.clients.Get(sender);
            if (c)
            {
                *c->input = incoming.input;
            }
            else
            {
                printf("Received update from dropped client\n");
            }
            break;
        }
        read_bytes = NetRead(buffer, max_size, &sender);
    }
}

int
main(int argc, char **argv)
{
    // TODO: Take from argv
    NetSetPreferredListenPort(12345);

    GameState state = {};
    InitGameState(state);

    uint64 initial_tick = GetTick();
    uint64 last_update_sent = initial_tick;
    uint64 last_game_tick = initial_tick;
    int tickrate = 20;
    int updaterate = 20;
    float tick_interval = 1.0f / float(tickrate);
    float update_interval = 1.0f / float(updaterate);
    int updates_sent = 0;
    while (1)
    {
        uint64 tick = GetTick();
        PollNetwork();

        if (TimeSince(last_game_tick) > tick_interval)
        {
            GameTick(state, app.clients.inputs, app.clients.count);
            last_game_tick = tick;
        }

        if (TimeSince(last_update_sent) > update_interval)
        {
            for (int i = 0; i < app.clients.count; i++)
            {
                updates_sent++;
                SendUpdate(state, app.clients.addresses[i]);
            }
            last_update_sent = tick;
        }

        NetStats stats = NetGetStats();
        printf("\rx = %d y = %d avg: %.2fKBps out, %.2fKBps in)",
                state.x, state.y,
                stats.avg_bytes_sent / 1024,
                stats.avg_bytes_read / 1024);
    }
}
