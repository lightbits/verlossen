#include <stdio.h>
#include "SDL.h"
#include "net.cpp"
#include "platform.cpp"
#include "test_game.cpp"

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int window_width;
    int window_height;
    const char *title;
    bool running;
    bool connected;
    NetAddress server;
};

static App app;

void
PollInput(GameInput &input)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            app.running = false;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            bool is_down = event.type == SDL_KEYDOWN;
            switch (event.key.keysym.sym)
            {
                case SDLK_z:     input.action1.is_down = is_down; break;
                case SDLK_x:     input.action2.is_down = is_down; break;
                case SDLK_LEFT:  input.left.is_down    = is_down; break;
                case SDLK_RIGHT: input.right.is_down   = is_down; break;
                case SDLK_UP:    input.up.is_down      = is_down; break;
                case SDLK_DOWN:  input.down.is_down    = is_down; break;
            }
            if (event.key.keysym.sym == SDLK_ESCAPE)
                app.running = false;
            break;
        }
    }
}

bool
CreateContext()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return false;

    app.window_width = 640;
    app.window_height = 320;
    app.title = "Verlossen";

    app.window = SDL_CreateWindow(
        app.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        app.window_width, app.window_height, SDL_WINDOW_OPENGL);
    if (!app.window)
        return false;

    app.renderer = SDL_CreateRenderer(
        app.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer)
        return false;

    return true;
}

void
SendInput(GameInput &input)
{
    ClientPacket p = {};
    p.protocol = CL_UPDATE;
    p.input = input;
    NetSend(&app.server, (const char *)&p, sizeof(ClientPacket));
}

void
SendConnect()
{
    ClientPacket p = {};
    p.protocol = CL_CONNECT;
    NetSend(&app.server, (const char*)&p, sizeof(ClientPacket));
}

int
main(int argc, char **argv)
{
    int listen_port = 54321;
    if (argc == 2)
    {
        sscanf(argv[1], "%d", &listen_port);
    }

    // TODO: Take from argv
    NetSetPreferredListenPort(listen_port);
    NetAddress server = {127, 0, 0, 1, 12345};

    if (!CreateContext())
        return false;

    GameInput input = {};
    GameState state = {};
    InitGameState(state);

    app.running = true;
    app.connected = false;
    app.server = server;
    int updaterate = 20;
    int updates_sent = 0;
    uint64 initial_tick = SDL_GetPerformanceCounter();
    uint64 last_connection_attempt = initial_tick;
    uint64 last_update_sent = initial_tick;
    uint64 last_update_recv = initial_tick;
    float server_timeout_interval = 2.0f;
    float connection_attempt_interval = 1.0f;
    float send_update_interval = 1.0f / float(updaterate);
    while (app.running)
    {
        uint64 frame_tick = SDL_GetPerformanceCounter();
        PollInput(input);

        ServerPacket update = {};
        char *buffer = (char*)&update;
        int max_size = sizeof(ServerPacket);
        int read_bytes = NetRead(buffer, max_size, NULL);
        while (read_bytes > 0)
        {
            last_update_recv = GetTick();
            switch (update.protocol)
            {
            case SV_ACCEPT:
                app.connected = true;
                break;
            case SV_UPDATE:
                state = update.state;
                break;
            }
            read_bytes = NetRead(buffer, max_size, NULL);
        }

        if (!app.connected &&
            TimeSince(last_connection_attempt) >
            connection_attempt_interval)
        {
            printf("Attempting to connect.\n");
            SendConnect();
            last_connection_attempt = frame_tick;
        }

        if (app.connected &&
            TimeSince(last_update_sent) >
            send_update_interval)
        {
            updates_sent++;
            SendInput(input);
            last_update_sent = frame_tick;
        }

        if (app.connected &&
            TimeSince(last_update_recv) >
            server_timeout_interval)
        {
            printf("Lost connection to server.\n");
            app.connected = false;
        }

        NetStats stats = NetGetStats();
        printf("\rx = %d y = %d avg: %.2fKBps out, %.2fKBps in)",
                state.x, state.y,
                stats.avg_bytes_sent / 1024,
                stats.avg_bytes_read / 1024);

        SDL_RenderPresent(app.renderer);
    }

    return true;
}
