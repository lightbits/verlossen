#include "client.h"

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    const char *title;
    int window_width;
    int window_height;
    int cl_updaterate;
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

static void
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

static void
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
SendConnect(NetAddress &server)
{
    ClientPacket p = {};
    p.rate = app.cl_updaterate;
    p.protocol = CL_CONNECT;
    NetSend(&server, (const char*)&p, sizeof(p));
}

void
SendUpdate(NetAddress &server, GameInput &input)
{
    ClientPacket p = {};
    p.rate = app.cl_updaterate;
    p.protocol = CL_UPDATE;
    p.input = input;
    NetSend(&server, (const char*)&p, sizeof(p));
}

void
SendDisconnect(NetAddress &server)
{
    ClientPacket p = {};
    p.protocol = CL_LOGOUT;
    NetSend(&server, (const char*)&p, sizeof(p));
}

static void
Client(NetAddress server, int listen_port, int updaterate)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return;

    app.cl_updaterate = updaterate;
    app.window_width = 640;
    app.window_height = 320;
    app.title = "Verlossen";

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

    GameMemory memory = {};
    memory.LoadTexture = PlatformLoadTexture;
    GameInit(memory);
    GameLoadTextures(memory);
    GameInput input = {};

    NetSetPreferredListenPort(listen_port);

    float  target_frame_time = 1.0f / 60.0f;
    uint64 initial_tick = SDL_GetPerformanceCounter();
    uint64 last_update  = initial_tick;
    uint64 frame_begin  = initial_tick;
    float  frame_time   = target_frame_time;
    float  elapsed_time = 0.0f;
    float  update_time  = 1.0f / float(updaterate);

    // Try to connect!
    bool connected = false;
    app.running = true;
    while (app.running)
    {
        HandleUserInput(input);

        uint64 tick = SDL_GetPerformanceCounter();
        if (GetElapsedTime(last_update, tick) >= update_time)
        {
            if (!connected)
            {
                SendConnect(server);
                printf("Attempting to connect\n");
            }
            else
            {
                SendUpdate(server, input);
            }
            last_update = tick;
        }

        NetAddress sender = {};
        ServerPacket p = {};
        int read_bytes = NetRead((char*)&p, sizeof(p), &sender);
        if (read_bytes > 0)
        {
            switch (p.protocol)
            {
                case SV_ACCEPT:
                    printf("Joined the game\n");
                    connected = true;
                    break;

                case SV_REJECT:
                    printf("Game was full\n");
                    break;

                case SV_UPDATE:
                    memory.state = p.state;
                    break;
            }
        }

        if (connected)
        {
            GameRender(memory, renderer);
        }
        else
        {
            RenderConnectionScreen(renderer, elapsed_time);
        }
        SDL_RenderPresent(app.renderer);

        uint64 frame_end = SDL_GetPerformanceCounter();
        elapsed_time = GetElapsedTime(initial_tick, frame_end);
        frame_time = GetElapsedTime(frame_begin, frame_end);
        frame_begin = frame_end;
    }

    SendDisconnect(server);
    SDL_Delay(500);
}
