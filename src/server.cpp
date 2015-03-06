#include "server.h"

static GameMemory memory;
static NetAddress clients_address[MAX_PLAYERS];
static GameInput clients_input[MAX_PLAYERS];
static int32     clients_rate[MAX_PLAYERS];
static uint64    clients_last_sent[MAX_PLAYERS];
static int client_count = 0;

void
Accept(
    ClientPacket &packet,
    NetAddress &sender)
{
    printf("%d.%d.%d.%d:%d wants to connect\n",
           sender.ip0, sender.ip1, sender.ip2, sender.ip3,
           sender.port);

    ServerPacket r = {};
    if (client_count == MAX_PLAYERS)
    {
        r.protocol = SV_REJECT;
    }
    else
    {
        GamePushPlayer(memory);
        clients_address[client_count] = sender;
        client_count++;
        r.protocol = SV_ACCEPT;
    }
    NetSend(&sender, (const char*)&r, sizeof(r));
}

void
Update(
    ClientPacket &packet,
    NetAddress &sender)
{
    for (int i = 0; i < client_count; i++)
    {
        if (NetAddrCmp(&sender, &clients_address[i]))
        {
            clients_input[i] = packet.input;
            clients_rate[i] = packet.rate;
            break;
        }
    }
}

void
Tick(float dt)
{
    const uint32 sz = sizeof(ClientPacket);
    NetAddress snd = {};
    char buf[sz];
    int read_cnt = NetRead(buf, sz, &snd);
    while (read_cnt > 0)
    {
        ClientPacket p = *(ClientPacket*)buf;
        switch (p.protocol)
        {
            case CL_CONNECT:
                Accept(p, snd);
                break;
            case CL_UPDATE:
                Update(p, snd);
                break;
        }
        read_cnt = NetRead(buf, sz, &snd);
    }

    GameUpdate(memory, clients_input, dt);
}

void
Server(int listen_port, int sv_tickrate)
{
    NetSetPreferredListenPort(listen_port);
    printf("Listening on port %d\n", listen_port);

    GameInit(memory);

    uint64 initial_tick = SDL_GetPerformanceCounter();
    uint64 last_cl_update = initial_tick;
    uint64 last_sv_tick = initial_tick;

    while (1)
    {
        uint64 tick = SDL_GetPerformanceCounter();
        float elapsed = GetElapsedTime(last_sv_tick, tick);
        float tick_time = 1.0f / float(sv_tickrate);
        if (elapsed > tick_time)
        {
            Tick(tick_time);
            tick = SDL_GetPerformanceCounter();
            last_sv_tick = tick;

            ServerPacket p = {};
            p.state = memory.state;
            p.protocol = SV_UPDATE;
            for (int i = 0; i < client_count; i++)
            {
                float since = GetElapsedTime(clients_last_sent[i], tick);
                float invrate = 1.0f / float(clients_rate[i]);
                if (since >= invrate)
                {
                    clients_last_sent[i] = tick;
                    NetAddress *dst = &clients_address[i];
                    NetSend(dst, (const char*)&p, sizeof(p));
                }
            }
        }
    }
}
