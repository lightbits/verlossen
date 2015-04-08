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
    int tickrate; // Updates/sec the game should run at
};

struct Network
{
    NetAddress server;
    bool connected;
    int cmd_rate;         // Updates/sec we send to server
    int rate;             // Updates/sec we want from server
    Sequence sequence;    // Tags next update we send to server
    Sequence expected;    // Expected tag of next update from server
    Sequence projected;   // Since we simulate ahead of the server...
    PlayerNum player_num; // We get this from the server when we connect
};

static Network net;
static App app;

void
PlatformRendererSetColor(uint32 color)
{
    SDL_SetRenderDrawColor(app.renderer,
        (color >> 24) & 0xff,
        (color >> 16) & 0xff,
        (color >> 8)  & 0xff,
        (color)       & 0xff);
}

void
PlatformRendererDrawLine(int x0, int y0, int x1, int y1)
{
    SDL_RenderDrawLine(app.renderer, x0, y0, x1, y1);
}

void
PlatformRendererClear()
{
    SDL_RenderClear(app.renderer);
}

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
    ClientCmd p = {};
    p.protocol = CL_UPDATE;
    p.expected = net.expected;
    p.input = input;
    p.rate = net.rate;
    NetSend(net.server, p);
}

void
SendConnect()
{
    ClientCmd p = {};
    p.protocol = CL_CONNECT;
    p.expected = net.expected;
    p.rate = net.rate;
    NetSend(net.server, p);
}

float
Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

GamePlayer
InterpolatePlayer(GamePlayer &a, GamePlayer &b, float t)
{
    GamePlayer result = a;
    result.x = Lerp(a.x, b.x, t);
    result.y = Lerp(a.y, b.y, t);
    return result;
}

GameState
InterpolateState(GameState a, GameState b, float t)
{
    GameState result = a;
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        result.players[i] = InterpolatePlayer(a.players[i], b.players[i], t);
    }
    return result;
}

GameState
PredictState(
    GameState   initial_state,
    GameInput  *global_inputs, // The other players's latest input
    PlayerNum   player_index,  // Our player index
    RingBuffer  input_history,
    int         rtt,
    float       dt)
{
    while (input_history.GetElementCount() > rtt)
        RingPopStruct(input_history, GameInput);
    GameState result = initial_state;
    for (int i = 0; i < rtt / 2; i++)
    {
        GameInput *input = RingPopStruct(input_history, GameInput);
        if (!input)
            break; // Not enough inputs in our history!
        global_inputs[player_index] = *input;
        GameTick(result, global_inputs, dt);
    }
    return result;
}

int
main(int argc, char **argv)
{
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
    net.server = NetMakeAddress(127, 0, 0, 1, 12345);

    if (!CreateContext())
        return -1;

    GameRenderer renderer = {};
    renderer.SetColor  = PlatformRendererSetColor;
    renderer.DrawLine  = PlatformRendererDrawLine;
    renderer.Clear     = PlatformRendererClear;
    // renderer.Blit      = PlatformBlit;
    renderer.res_x     = 320;
    renderer.res_y     = 160;
    SDL_RenderSetLogicalSize(app.renderer, renderer.res_x, renderer.res_y);

    GameMemory memory = {};
    // memory.LoadTexture = PlatformLoadTexture;
    GameInit(memory.state);

    GameInput input = {};

    GameState state_server    = {};
    GameState state_predicted = {};

    const int MAX_INPUT_HISTORY = 32;
    GameInput input_buffer[MAX_INPUT_HISTORY];
    RingBuffer input_history = MakeRingBuffer(
       (uint8*)input_buffer, MAX_INPUT_HISTORY);

    app.running = true;
    net.connected = false;
    app.tickrate = 20;
    uint64 initial_tick               = SDL_GetPerformanceCounter();
    uint64 last_connection_attempt    = initial_tick;
    uint64 last_update_sent           = initial_tick;
    uint64 last_update_recv           = initial_tick;
    uint64 last_game_tick             = initial_tick;
    float tick_interval               = 1.0f / float(app.tickrate);
    float send_update_interval        = 1.0f / float(net.cmd_rate);
    float connection_attempt_interval = 1.0f;
    float server_timeout_interval     = 2.0f;
    while (app.running)
    {
        uint64 frame_tick = SDL_GetPerformanceCounter();
        PollInput(input);

        GameInput *next = RingPushStruct(input_history, GameInput);
        if (!next)
        {
            RingPopStruct(input_history, GameInput);
            next = RingPushStruct(input_history, GameInput);
        }
        *next = input;

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

        static int estimated_rtt = 12; // in ticks

        NetAddress sender = {};
        ServerUpdate update = {};
        while (NetRead(update, sender))
        {
            last_update_recv = GetTick();
            switch (update.protocol)
            {
            case SV_ACCEPT:
                net.expected = update.sequence + 1;
                net.connected = true;
                net.player_num = update.player_num;
                break;

            case SV_UPDATE:
                if (!IsPacketMoreRecent(net.expected, update.sequence))
                    break;

                net.expected = update.sequence + 1;

                state_server = update.state;
                state_predicted = PredictState(update.state,
                               update.inputs,
                               net.player_num,
                               input_history,
                               estimated_rtt,
                               tick_interval);

                break;
            }
        }

        if (input.action1.is_down)
            memory.state = state_server;

        // if (input.action1.is_down)
        // {
        //     estimated_rtt++;
        //     printf("horizon = %d\n", estimated_rtt);
        // }
        // if (input.action2.is_down && estimated_rtt > 0)
        // {
        //     estimated_rtt--;
        //     printf("horizon = %d\n", estimated_rtt);
        // }

        if (net.connected &&
            TimeSince(last_game_tick) > tick_interval)
        {
            net.projected++;
            update.inputs[net.player_num] = input;
            last_game_tick = frame_tick;
            GameTick(memory.state, update.inputs, tick_interval);
        }

        renderer.SetColor(PAL16_VOID);
        renderer.Clear();
        DebugGameRender(memory.state, renderer, PAL16_BLAZE);
        DebugGameRender(state_server, renderer, PAL16_ASH);
        DebugGameRender(state_predicted, renderer, PAL16_SEABLUE);
        // GameRender(memory, renderer);

        SDL_RenderPresent(app.renderer);

        // NetStats stats = NetGetStats();
        // printf("\r");
        // printf("x = %.2f", memory.state.players[0].x);
        // printf("\tavg %.2f KBps out", stats.avg_bytes_sent / 1024);
        // printf("\t%.2f KBps in", stats.avg_bytes_read / 1024);
        // printf("\tahead by %.2f ms   ",
        //     1000.0f * float(net.projected - net.expected) * tick_interval);
    }

    return 0;
}
