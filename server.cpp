#include "server.h"

void
HandleClientConnection(GameMemory &memory,
                       ClientPacket &packet,
                       NetAddress &sender)
{
    printf("%d.%d.%d.%d:%d wants to connect (%s)\n",
           sender.ip0, sender.ip1, sender.ip2, sender.ip3,
           sender.port, packet.data);

    ServerPacket response = {};
    response.protocol = SV_ACCEPT;
    sprintf(response.data, "Connection accepted.");
    NetSend(&sender, (const char*)&response, sizeof(ServerPacket));
}

void
HandleClientUpdate(GameMemory &memory,
                   ClientPacket &packet,
                   NetAddress &sender)
{
    printf("UPDATE FROM %d.%d.%d.%d:%d\n",
       sender.ip0, sender.ip1, sender.ip2, sender.ip3,
       sender.port);
}

void
Server(int listen_port)
{
    NetSetPreferredListenPort(listen_port);
    printf("Listening on port %d\n", listen_port);

    GameMemory memory = {};
    memory.LoadTexture = PlatformLoadTexture;
    GameInit(memory);

    int sv_tickrate = 66;
    float sv_tick_time = 1.0f / float(sv_tickrate);
    uint64 initial_tick = SDL_GetPerformanceCounter();

    // TODO: Update rates should be given by the clients
    // themselves, as a client may want to have a higher rate!
    // That means we need to track the last time we sent an
    // update to each client.
    int cl_updaterate = 66;
    float cl_update_time = 1.0f / float(cl_updaterate);

    uint64 last_cl_update = initial_tick;
    uint64 last_sv_tick = initial_tick;

    while (1)
    {
        uint64 tick = SDL_GetPerformanceCounter();

        if (GetElapsedTime(last_sv_tick, tick) > sv_tick_time)
        {
            NetAddress sender = {};
            char buffer[sizeof(ClientPacket)];
            int read_bytes = NetRead(buffer, sizeof(ClientPacket), &sender);
            while (read_bytes > 0)
            {
                ClientPacket packet = *(ClientPacket*)buffer;
                switch (packet.protocol)
                {
                    case CL_CONNECT:
                        HandleClientConnection(memory, packet, sender);
                        break;
                    case CL_UPDATE:
                        HandleClientUpdate(memory, packet, sender);
                        break;
                }
                read_bytes = NetRead(buffer, sizeof(ClientPacket), &sender);
            }

            tick = SDL_GetPerformanceCounter();
            last_sv_tick = tick;
        }

        if (GetElapsedTime(last_cl_update, tick) > cl_update_time)
        {
            tick = SDL_GetPerformanceCounter();
            last_cl_update = tick;
        }
    }
}
