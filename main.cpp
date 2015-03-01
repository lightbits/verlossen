#include <stdio.h>
#include "SDL.h"
#include "net.h"
#include "game.h"
#include "matrix.cpp"
#include "game.cpp"
// #include "win32_net.cpp"

const int CL_LISTEN_PORT = 12345;
const int SV_TICKRATE = 20;
const int CL_CMDRATE = 20;
const int CL_UPDATERATE = 20;
const int CL_PACKET_SIZE = 64;
const int SV_PACKET_SIZE = 64;
const int SV_LISTEN_PORT = 65387;
const int SV_LISTEN_IP0  = 127;
const int SV_LISTEN_IP1  = 0;
const int SV_LISTEN_IP2  = 0;
const int SV_LISTEN_IP3  = 1;

// Message types
const uint32 APP_PROTOCOL   = 0xDEADBEEF;
const uint32 APP_CONNECT    = 0x00000001;
const uint32 APP_DISCONNECT = 0x00000002;

float GetElapsedTime(uint64 begin, uint64 end)
{
    uint64 frequency = SDL_GetPerformanceFrequency();
    return (float)(end - begin) / (float)frequency;
}

struct ConnectionResponse
{
    uint32 protocol;
    char welcome[SV_PACKET_SIZE - sizeof(uint32)];
};

// struct ConnectionRequest
// {
//     uint32 protocol;
//     uint32 regards;
//     char   hail[CL_PACKET_SIZE - 2 * sizeof(uint32)];
// };

// struct DisconnectRequest
// {
//     uint32 protocol;
//     uint32 regards;
// };

// ConnectionResponse
// ClientConnect(NetAddress *server)
// {
//     ConnectionRequest request = {};
//     request.protocol = APP_PROTOCOL;
//     request.regards  = APP_CONNECT;

//     // TODO: Replace with safer snprintf
//     sprintf(request.hail, "Client says hi!");
//     NetAddress dst = {
//         SV_LISTEN_IP0,
//         SV_LISTEN_IP1,
//         SV_LISTEN_IP2,
//         SV_LISTEN_IP3,
//         SV_LISTEN_PORT
//     };

//     ConnectionResponse response = {};
//     int read_bytes = 0;
//     int max_read_tries = 5;
//     printf("Attempting to connect to %d.%d.%d.%d:%d\n",
//         dst.ip0, dst.ip1, dst.ip2, dst.ip3, dst.port);
//     while (read_bytes <= 0)
//     {
//         // Send our connection request!
//         NetSend(&dst, (char*)&request, sizeof(ConnectionRequest));

//         int read_tries = 0;
//         while (read_bytes <= 0 && read_tries < max_read_tries)
//         {
//             read_bytes = NetRead(
//                 (char*)&response, sizeof(ConnectionResponse), server);

//             if (response.protocol == APP_PROTOCOL)
//             {
//                 break;
//             }
//             else
//             {
//                 read_tries++;
//                 SDL_Delay(1000);
//             }
//         }

//         if (read_bytes <= 0)
//             printf("Retrying connection\n");
//     }
//     return response;
// }

// void
// ClientDisconnect(NetAddress *server)
// {
//     char data[] = {APP_PROTOCOL, APP_DISCONNECT};
//     NetSend(server, data, sizeof(data));
//     printf("Disconnecting!");
//     SDL_Delay(1000);
// }

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

struct Message
{
    uint32 protocol;
    uint16 flag;
    char hail[256];
};

#define RGB(hex) (hex >> 24) & 0xff, (hex >> 16) & 0xff, (hex >> 8) & 0xff, (hex) & 0xff

void
PlatformRendererSetColor(uint32 color)
{
    SDL_SetRenderDrawColor(app.renderer, RGB(color));
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
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 0, 255));
    if (!surface)
    {
        SDL_Log("Asset \"%s\" could not be found in data directory",
            asset_name);
        exit(-1);
    }
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
    int dst_x, int dst_y, int dst_w, int dst_h)
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
    SDL_RenderCopy(app.renderer, (SDL_Texture*)texture.data, &src, &dst);
}

int
wmain(int argc, wchar_t **argv)
{
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
    renderer.SetColor   = PlatformRendererSetColor;
    renderer.DrawLine   = PlatformRendererDrawLine;
    renderer.Clear      = PlatformRendererClear;
    renderer.Blit       = PlatformBlit;
    renderer.res_x      = 320;
    renderer.res_y      = 160;
    SDL_RenderSetLogicalSize(app.renderer, renderer.res_x, renderer.res_y);

    GameMemory memory   = {};
    memory.frame_time   = 1.0f / 60.0f;
    memory.elapsed_time = 0.0f;
    memory.renderer     = renderer;
    memory.LoadTexture  = PlatformLoadTexture;

    GameInput input = {};

    app.running = true;
    float target_frame_time = 1.0f / 60.0f;
    uint64 game_begin = SDL_GetPerformanceCounter();
    uint64 frame_begin = game_begin;
    while (app.running) {
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
                    case SDLK_z:     input.btn_action.is_down = is_down; break;
                    case SDLK_LEFT:  input.btn_left.is_down   = is_down; break;
                    case SDLK_RIGHT: input.btn_right.is_down  = is_down; break;
                    case SDLK_UP:    input.btn_up.is_down     = is_down; break;
                    case SDLK_DOWN:  input.btn_down.is_down   = is_down; break;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    app.running = false;
                break;
            }
        }

        GameUpdateAndRender(memory, input);
        SDL_RenderPresent(app.renderer);

        uint64 frame_end = SDL_GetPerformanceCounter();
        memory.elapsed_time = GetElapsedTime(game_begin, frame_end);
        memory.frame_time = GetElapsedTime(frame_begin, frame_end);
        frame_begin = frame_end;
    }

    return 0;
}
