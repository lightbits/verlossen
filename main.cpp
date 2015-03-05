#include <stdio.h>
#include "SDL.h"
#include "game.h"
#include "matrix.cpp"
#include "game.cpp"
#include "net.h"
#include "win32_net.cpp"
#include "palette.h"
#define CL_UPDATE_RATE 20

float GetElapsedTime(uint64 begin, uint64 end)
{
    uint64 frequency = SDL_GetPerformanceFrequency();
    return (float)(end - begin) / (float)frequency;
}

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    const char *title;
    int window_width;
    int window_height;
    bool running;
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

struct RingBuffer
{
    int read_index;
    int write_index;
    uint32 max_data_count;
    uint8 *data;

    void *Push(uint32 size)
    {
        if (write_index == -1)
            return 0;
        if (read_index == -1)
            read_index = write_index;
        void *result = data + write_index * size;
        write_index = (write_index + 1) % max_data_count;
        if (write_index == read_index)
            write_index = -1;
        return result;
    }

    void *Pop(uint32 size)
    {
        if (read_index == -1)
            return 0;
        if (write_index == -1)
            write_index = read_index;
        void *result = data + read_index * size;
        read_index = (read_index + 1) % max_data_count;
        if (read_index == write_index)
            read_index = -1;
        return result;
    }
};

RingBuffer
MakeRingbuffer(uint8 *buffer, uint32 max_data_count)
{
    RingBuffer result = {};
    result.data = buffer;
    result.max_data_count = max_data_count;
    result.read_index = -1;
    return result;
}

#define PopStruct(rb, type) ((type*)rb.Pop(sizeof(type)))
#define PushStruct(rb, type) ((type*)rb.Push(sizeof(type)))

#define CL_CONNECT 0xDEADBEEF
#define CL_UPDATE  0xABABABAB
#define SV_ACCEPT  0xABAD1DEA
#define SV_UPDATE  0xFABFABFA
struct ClientPacket
{
    int32 protocol;
    char  data[256];
};

struct ServerPacket
{
    int32 protocol;
    char  data[1024];
};

void
Server(int listen_port)
{
    NetSetPreferredListenPort(listen_port);
    printf("Listening on port %d\n", listen_port);

    while (1)
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
                {
                    printf("%d.%d.%d.%d:%d wants to connect (%s)\n",
                           sender.ip0, sender.ip1, sender.ip2, sender.ip3,
                           sender.port, packet.data);

                    ServerPacket response = {};
                    response.protocol = SV_ACCEPT;
                    sprintf(response.data, "Connection accepted.");
                    printf("Sending %d %s\n", response.protocol, response.data);
                    NetSend(&sender, (const char*)&response, sizeof(ServerPacket));
                } break;
                case CL_UPDATE:
                {

                } break;
            }

            read_bytes = NetRead(buffer, sizeof(ClientPacket), &sender);
        }
    }
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

void
RenderConnectionScreen(GameRenderer &render, float elapsed_time)
{
    float speed = 8.0f;
    int mod = int(speed * elapsed_time) % 16;
    render.SetColor(PAL16_VOID);
    render.Clear();
    for (int i = 0; i < 16; i++)
    {
        int seg_width = 16;
        int seg_height = 2;
        int left = (render.res_x / 2) - 8 * seg_width;
        int top = (render.res_y / 2) - seg_height / 2;
        int bottom = (render.res_y / 2) + seg_height / 2;
        render.SetColor(PAL16[(i + mod) % 16]);
        for (int r = 0; r < seg_width; r++)
        {
            int x = left + i * seg_width + r;
            render.DrawLine(x, top, x, bottom);
        }
    }
}

void
Client(NetAddress server_addr, int listen_port)
{
    app.window = SDL_CreateWindow(
        app.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        app.window_width, app.window_height, SDL_WINDOW_OPENGL);
    if (!app.window)
        return;

    app.renderer = SDL_CreateRenderer(
        app.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer)
        return;

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
    GameInit(memory);

    GameInput input    = {};

    NetSetPreferredListenPort(listen_port);

    ClientPacket request = {};
    request.protocol = CL_CONNECT;
    sprintf(request.data, "Client says hello!");

    float  target_frame_time = 1.0f / 60.0f;
    uint64 game_begin     = SDL_GetPerformanceCounter();
    uint64 frame_begin    = game_begin;
    uint64 prev_net_send  = game_begin;
    float  frame_time     = target_frame_time;
    float  elapsed_time   = 0.0f;
    float  retry_interval = 1.0f;

    bool connected = false;

    // Try to connect!
    app.running = true;
    while (app.running)
    {
        HandleUserInput(input);

        uint64 curr_tick = SDL_GetPerformanceCounter();
        if (GetElapsedTime(prev_net_send, curr_tick) >= retry_interval &&
            !connected)
        {
            printf("Attempting to connect\n");
            NetSend(&server_addr, (const char*)&request, sizeof(ClientPacket));
            prev_net_send = curr_tick;
        }

        NetAddress sender = {};
        char buffer[sizeof(ServerPacket)];
        int read_bytes = NetRead(buffer, sizeof(ServerPacket), &sender);
        if (read_bytes > 0)
        {
            printf("Read some bytes\n");
            ServerPacket packet = *(ServerPacket*)buffer;
            switch (packet.protocol)
            {
                case SV_ACCEPT:
                {
                    printf("Connected to %d.%d.%d.%d:%d (%s)\n",
                           sender.ip0, sender.ip1, sender.ip2, sender.ip3,
                           sender.port, packet.data);
                    connected = true;
                } break;

                case SV_UPDATE:
                {
                    printf("Received update.\n");
                } break;
            }
        }

        RenderConnectionScreen(renderer, elapsed_time);
        SDL_RenderPresent(app.renderer);

        uint64 frame_end = SDL_GetPerformanceCounter();
        elapsed_time = GetElapsedTime(game_begin, frame_end);
        frame_time = GetElapsedTime(frame_begin, frame_end);
        frame_begin = frame_end;
    }
}

int
main(int argc, char *argv[])
{
    // if (argc == 3)
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

    // TODO: Take in server ip and listen port and stuff?
    bool is_server = false;
    if (argc == 2)
    {
        is_server = true;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return -1;

    app.window_width = 640;
    app.window_height = 320;
    app.title = "Verlossen";

    // TODO: Server should also open window...?
    if (is_server)
        Server(12345);
    else
    {
        NetAddress server_addr = {127, 0, 0, 1, 12345};
        Client(server_addr, 54321);
    }

    return 0;
}
