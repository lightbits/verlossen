#include <stdio.h>
#include "SDL.h"
#include "game.h"
#include "matrix.cpp"
#include "game.cpp"
#include "net.h"
// #include "win32_net.cpp"

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
main(int argc, char *argv[])
{
    // printf("%d", argc);
    // printf("%s", argv[1]);
    // printf("hi\n");
    // float f1, f2;
    // sscanf((const char*)argv[1], "[%g , %g]", &f1, &f2);
    // printf("%f, %f", f1, f2);
    // NetSetPreferredListenPort(12345);

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
                    case SDLK_z:     input.action1.is_down = is_down; break;
                    case SDLK_x:     input.action2.is_down = is_down; break;
                    case SDLK_LEFT:  input.left.is_down    = is_down; break;
                    case SDLK_RIGHT: input.right.is_down   = is_down; break;
                    case SDLK_UP:    input.up.is_down      = is_down; break;
                    case SDLK_DOWN:  input.down.is_down    = is_down; break;

                    // NetAddress dst = {127, 0, 0, 1, }
                    // NetSend()
                }
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    app.running = false;
                break;
            }
        }

        GameUpdateAndRender(memory, renderer, input);
        SDL_RenderPresent(app.renderer);

        uint64 frame_end = SDL_GetPerformanceCounter();
        input.elapsed_time = GetElapsedTime(game_begin, frame_end);
        input.frame_time = GetElapsedTime(frame_begin, frame_end);
        frame_begin = frame_end;
    }

    return 0;
}
