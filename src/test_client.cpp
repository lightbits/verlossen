#include <stdio.h>
#include "SDL.h"
#include "net.cpp"
#include "platform.cpp"
#include "test_game.cpp"
#include "buffer.cpp"

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int window_width;
    int window_height;
    const char *title;
    bool running;
};

struct Network
{
    NetAddress server;
    bool connected;
    int cmd_rate;    // Updates/sec we send to server
    int rate;        // Updates/sec we want from server
    Sequence sequence; // Tags next update we send to server
    Sequence expected; // Expected tag of next update from server
};

static Network net;
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
    p.rate = net.rate;
    p.expected = net.expected;
    NetSend(&net.server, (const char *)&p, sizeof(ClientPacket));
}

void
SendConnect()
{
    ClientPacket p = {};
    p.protocol = CL_CONNECT;
    p.rate = net.rate;
    p.expected = net.expected;
    NetSend(&net.server, (const char*)&p, sizeof(ClientPacket));
}

int
LerpInt(int a, int b, float t)
{
    return int(a + (b - a) * t);
}

// GameState
// InterpolateState(
//     GameState a,
//     GameState b,
//     float alpha)
// {
//     GameState result = {};
//     a.x = LerpInt(a.x, b.x, alpha);
//     b.y = LerpInt(a.y, b.y, alpha);
//     return result;
// }

// // TODO: Implement this!!
// GameState
// PredictState(
//     GameState initial_state,
//     RingBuffer local_inputs, // Deliberately a copy
//     int horizon)
// {
//     // TODO: Other player's input. assume constant over horizon?
//     GameInput inputs[4]; // TODO: Define this param?
//     // TODO: Need our "input_index" to "player index" mapping
//     GameState result = initial_state;
//     for (int i = 0; i < horizon; i++)
//     {
//         GameInput *input = RingPopStruct(local_inputs, GameInput);
//         // tick game, based on everyone elses input as well
//     }
//     return result;
// }

int
main(int argc, char **argv)
{
    // TODO: Load config in a better way
    int listen_port = 54321;
    net.cmd_rate = 20;
    net.rate = 20;
    if (argc >= 2)
    {
        sscanf(argv[1], "%d", &listen_port);
    }
    if (argc == 4)
    {
        sscanf(argv[2], "%d", &net.cmd_rate);
        sscanf(argv[3], "%d", &net.rate);
    }
    NetSetPreferredListenPort(listen_port);

    // TODO: Let user set this in a screen?
    net.server = NetMakeAddress(127, 0, 0, 1, 12345);

    if (!CreateContext())
        return false;

    GameInput input = {};
    GameState state = {};
    GameInit(state);

    // const int LERP_WINDOW = 16; // Should be estimated from RTT
    // GameInput input_buffer[LERP_WINDOW];
    // RingBuffer input_ring = MakeRingBuffer((uint8*)input_buffer, LERP_WINDOW);

    app.running = true;
    net.connected = false;
    int updates_sent = 0;
    uint64 initial_tick = SDL_GetPerformanceCounter();
    uint64 last_connection_attempt = initial_tick;
    uint64 last_update_sent = initial_tick;
    uint64 last_update_recv = initial_tick;
    float server_timeout_interval = 2.0f;
    float connection_attempt_interval = 1.0f;
    float send_update_interval = 1.0f / float(net.cmd_rate);
    while (app.running)
    {
        uint64 frame_tick = SDL_GetPerformanceCounter();
        PollInput(input);

        // GameInput *next = RingPushStruct(input_ring, GameInput);
        // if (!next)
        // {
        //     RingPopStruct(input_ring, GameInput);
        //     next = RingPushStruct(input_ring, GameInput);
        // }
        // *next = input;

        // printf("%d\n", input_ring.write_index);

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
                net.expected = update.sequence + 1;
                net.connected = true;
                break;
            case SV_UPDATE:
                if (IsPacketMoreRecent(
                    net.expected,
                    update.sequence))
                {
                    net.expected = update.sequence + 1;

                    // TODO: Sync state, simulation, prediction
                    // printf("Popping input\n");
                    // RingPopStruct(input_ring, GameInput);
                    state = update.state;
                }
                break;
            }
            read_bytes = NetRead(buffer, max_size, NULL);
        }

        if (!net.connected &&
            TimeSince(last_connection_attempt) >
            connection_attempt_interval)
        {
            printf("Attempting to connect.\n");
            SendConnect();
            last_connection_attempt = frame_tick;
        }

        if (net.connected &&
            TimeSince(last_update_sent) >
            send_update_interval)
        {
            updates_sent++;
            SendInput(input);
            last_update_sent = frame_tick;
        }

        if (net.connected &&
            TimeSince(last_update_recv) >
            server_timeout_interval)
        {
            printf("Lost connection to server.\n");
            net.connected = false;
        }

        NetStats stats = NetGetStats();
        // printf("\rx = %d y = %d", state.x, state.y);
        printf("\r");
        printf("\tavg %.2f KBps out", stats.avg_bytes_sent / 1024);
        printf("\t%.2f KBps in", stats.avg_bytes_read / 1024);
        printf("\tlast recv %d", net.expected - 1);

        SDL_RenderPresent(app.renderer);
    }

    return true;
}
