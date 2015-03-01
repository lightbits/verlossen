#ifndef _game_h_
#define _game_h_
#include "matrix.h"
#include "platform.h"

struct GamePlayer
{
    vec2 position;
    vec2 velocity;
    vec2 size;
};

struct GameState
{
    GamePlayer player;
};

typedef void renderer_set_color(uint32 color);
typedef void renderer_draw_line(int x0, int y0, int x1, int y1);
typedef void renderer_clear();
struct GameRenderer
{
    renderer_set_color *SetColor;
    renderer_draw_line *DrawLine;
    renderer_clear     *Clear;
};

struct GameMemory
{
    bool         is_initialized;
    float        frame_time;
    float        elapsed_time;
    GameRenderer renderer;
};

struct GameFramebuffer
{
    // Pixels are always rgba (32 bit)
    uint32 *pixels;
    int     width;
    int     height;
};

struct GameButton
{
    bool is_down;
};

struct GameInput
{
    GameButton btn_action;
    GameButton btn_left;
    GameButton btn_right;
    GameButton btn_up;
    GameButton btn_down;
};

void GameLoad();
void GameUpdateAndRender(GameFramebuffer &framebuffer,
                         GameMemory &memory,
                         GameInput &input);

#endif
