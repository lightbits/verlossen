#include <stdio.h>
#include "client_test.cpp"
#include "server_test.cpp"
#include "net.cpp"
// #include "server.h"
// #include "client.h"
// #include "client.cpp"
// #include "server.cpp"
// #include "matrix.cpp"
// #include "game.cpp"
// #include "net.cpp"

uint64
GetTick()
{
    return SDL_GetPerformanceCounter();
}

float
GetElapsedTime(uint64 begin, uint64 end)
{
    uint64 frequency = SDL_GetPerformanceFrequency();
    return (float)(end - begin) / (float)frequency;
}

float
TimeSince(uint64 then)
{
    uint64 now = GetTick();
    return GetElapsedTime(then, now);
}

int
main(int argc, char *argv[])
{
    // TODO: Take in server ip and listen port and stuff
    // if (argc == 3)<
    // {
    //     sscanf(argv[1], "%d", &preferred_listen_port);

    //     sscanf(argv[2], "%d.%d.%d.%d:%d",
    //            &dst.ip0, &dst.ip1, &dst.ip2, &dst.ip3,
    //            &dst.port);

    //     printf("%d %d.%d.%d.%d:%d",
    //            preferred_listen_port,
    //            dst.ip0, dst.ip1, dst.ip2, dst.ip3, dst.port);
    // }
    // else
    // {
    //     printf("Format:\n12345 201.220.241.172:54321\n");
    // }

    bool is_server = false;
    if (argc == 2)
    {
        NetSetPreferredListenPort(12345);
        Server();
        // int tick_rate = 66;
        // Server(12345, tick_rate);
    }
    else
    {
        NetSetPreferredListenPort(54321);
        NetAddress server = {127, 0, 0, 1, 12345};
        Client(server);
        // int updaterate = 66;
        // Client(server_addr, 54321, updaterate);
    }

    return 0;
}
