#include <stdio.h>
#include "SDL.h"
#include "game.h"
#include "matrix.cpp"
#include "game.cpp"
#include "net.h"
#include "win32_net.cpp"
#define CL_UPDATE_RATE 20

float GetElapsedTime(uint64 begin, uint64 end)
{
    uint64 frequency = SDL_GetPerformanceFrequency();
    return (float)(end - begin) / (float)frequency;
}

enum AppMode
{
    AppClient,
    AppServer
};

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    const char *title;
    int window_width;
    int window_height;
    bool running;
    AppMode mode;
};

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

GameTexture
PlatformLoadTexture(const char *asset_name)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    char path[1024] = {};
    sprintf(path, "./data/%s", asset_name);
    surface = SDL_LoadBMP(path);
    if (!surface)
    {
        SDL_Log("Asset \"%s\" could not be found in data directory",
            asset_name);
        exit(-1);
    }
    uint32 magenta = SDL_MapRGB(surface->format, 255, 0, 255);
    SDL_SetColorKey(surface, SDL_TRUE, magenta);
    texture = SDL_CreateTextureFromSurface(app.renderer, surface);
    SDL_FreeSurface(surface);

    GameTexture result = {};
    SDL_QueryTexture(texture, 0, 0, &result.width, &result.height);
    result.data = (void*)texture;
    return result;
}

void
PlatformBlit(GameTexture texture,
    int src_x, int src_y, int src_w, int src_h,
    int dst_x, int dst_y, int dst_w, int dst_h,
    BlitFlip flip)
{
    SDL_Rect src;
    src.x = src_x;
    src.y = src_y;
    src.w = src_w;
    src.h = src_h;
    SDL_Rect dst;
    dst.x = dst_x;
    dst.y = dst_y;
    dst.w = dst_w;
    dst.h = dst_h;
    if (flip == BlitFlipNone)
        SDL_RenderCopy(app.renderer, (SDL_Texture*)texture.data, &src, &dst);
    else if (flip == BlitFlipHorizontal)
        SDL_RenderCopyEx(app.renderer, (SDL_Texture*)texture.data,
                         &src, &dst, 0.0, 0, SDL_FLIP_HORIZONTAL);
}

void
HandleUserInput(GameInput &input)
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

struct RingBuffer
{
    int read_index;
    int write_index;
    int *data; // generic pointer to externally allocated array
    int size;  // size of that array

    bool Push(int i)
    {
        if (write_index == -1)
            return false;
        if (read_index == -1)
            read_index = write_index;
        data[write_index] = i;
        write_index = (write_index + 1) % size;
        if (write_index == read_index)
            write_index = -1;
        return true;
    }

    bool Pop(int *i)
    {
        if (read_index == -1)
            return false;
        if (write_index == -1)
            write_index = read_index;
        *i = data[read_index];
        data[read_index] = 0;
        read_index = (read_index + 1) % size;
        if (read_index == write_index)
            read_index = -1;
        return true;
    }
};

RingBuffer
MakeRingbuffer(int *data, int size)
{
    RingBuffer result = {};
    result.data = data;
    result.size = size;
    result.read_index = -1;
    return result;
}

void
PrintRingBuffer(RingBuffer &rb)
{
    for (int i = 0; i < rb.size; i++)
    {
        if (i == rb.read_index)
            printf("r");
        else if (i == rb.write_index)
            printf("w");
        else
            printf(" ");
    }
    printf("\n");
    for (int i = 0; i < rb.size; i++)
        printf("%d", rb.data[i]);
    printf("\n");
}

int
main(int argc, char *argv[])
{
    NetAddress dst = {127, 0, 0, 1, 27050};
    int preferred_listen_port = 27050;
    if (argc == 3)
    {
        sscanf(argv[1], "%d", &preferred_listen_port);

        sscanf(argv[2], "%d.%d.%d.%d:%d",
               &dst.ip0, &dst.ip1, &dst.ip2, &dst.ip3,
               &dst.port);

        printf("%d %d.%d.%d.%d:%d",
               preferred_listen_port,
               dst.ip0, dst.ip1, dst.ip2, dst.ip3, dst.port);
    }
    else
    {
        printf("Format:\n12345 201.220.241.172:54321\n");
    }

    NetSetPreferredListenPort(preferred_listen_port);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return -1;

    app.window_width = 640;
    app.window_height = 320;
    app.title = "Verlossen";

    app.window = SDL_CreateWindow(
        app.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        app.window_width, app.window_height, SDL_WINDOW_OPENGL);
    if (!app.window)
        return -1;

    app.renderer = SDL_CreateRenderer(
        app.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer)
        return -1;

    GameRenderer renderer = {};
    renderer.SetColor  = PlatformRendererSetColor;
    renderer.DrawLine  = PlatformRendererDrawLine;
    renderer.Clear     = PlatformRendererClear;
    renderer.Blit      = PlatformBlit;
    renderer.res_x     = 320;
    renderer.res_y     = 160;
    SDL_RenderSetLogicalSize(app.renderer, renderer.res_x, renderer.res_y);

    GameMemory memory  = {};
    memory.LoadTexture = PlatformLoadTexture;

    GameInput input    = {};
    input.frame_time   = 1.0f / 60.0f;
    input.elapsed_time = 0.0f;

    /////////////
    int data[4] = {0, 0, 0, 0};
    int temp;
    RingBuffer rb = MakeRingbuffer(data, 4);
    rb.Push(1); PrintRingBuffer(rb);
    rb.Push(2); PrintRingBuffer(rb); rb.Pop(&temp); PrintRingBuffer(rb);
    rb.Push(3); PrintRingBuffer(rb);
    rb.Push(4); PrintRingBuffer(rb); rb.Pop(&temp); PrintRingBuffer(rb);
    rb.Pop(&temp); PrintRingBuffer(rb);
    rb.Pop(&temp); PrintRingBuffer(rb);
    rb.Push(5); PrintRingBuffer(rb);
    rb.Push(6); PrintRingBuffer(rb); rb.Pop(&temp); rb.Pop(&temp); rb.Pop(&temp); rb.Pop(&temp); PrintRingBuffer(rb);
    rb.Push(7); PrintRingBuffer(rb);
    rb.Push(8); PrintRingBuffer(rb);

    /*
    Should the server advance all ringbuffers of user
    inputs in lockstep, without regard for individual
    differences in latency?

    Can the server just speed thru a single client's
    ringbuffer?

    Server:
        forever:
            for client in clients
                for cmd in client.buffer
                    StepWorld(cmd)

    Probably not. I think stepping the world requires
    all clients' inputs. Like:

    Server:
        forever:
            inputs = {}
            for client in clients:
                inputs[client] = client.buffer.read_one()
            StepWorld(inputs)

    */

    /////////////

    app.running = true;
    float target_frame_time = 1.0f / 60.0f;
    uint64 game_begin = SDL_GetPerformanceCounter();
    uint64 frame_begin = game_begin;
    uint64 prev_net_update_tick = frame_begin;
    while (app.running) {
        HandleUserInput(input);

        char buffer[sizeof(GameNetworkPacket)];
        NetAddress src = {};
        int bytes_read = NetRead(buffer, sizeof(GameNetworkPacket), &src);

        // Should this be a while loop?
        // if (bytes_read > 0)
        // {
        //     GameNetworkPacket packet = *(GameNetworkPacket*)buffer;
        //     memory.network.incoming = packet;
        //     memory.network.fresh_update = true;
        // }
        // else
        // {
        //     memory.network.fresh_update = false;
        // }

        GameUpdateAndRender(memory, renderer, input);
        SDL_RenderPresent(app.renderer);

        uint64 frame_end = SDL_GetPerformanceCounter();
        input.elapsed_time = GetElapsedTime(game_begin, frame_end);
        input.frame_time = GetElapsedTime(frame_begin, frame_end);

        // if (GetElapsedTime(prev_net_update_tick, frame_end) >
        //     1.0f / CL_UPDATE_RATE)
        // {
        //     NetSend(&dst, (char*)&memory.network.outgoing,
        //         sizeof(GameNetworkPacket));
        //     prev_net_update_tick = frame_end;
        // }

        frame_begin = frame_end;
    }

    return 0;
}
