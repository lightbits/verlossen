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

struct GameTexture
{
    int width;
    int height;
    void *data;
};

typedef void renderer_set_color(uint32 color);
typedef void renderer_draw_line(int x0, int y0, int x1, int y1);
typedef void renderer_clear();
typedef void renderer_blit(GameTexture texture,
    int src_x, int src_y, int src_w, int src_h,
    int dst_x, int dst_y, int dst_w, int dst_h);
struct GameRenderer
{
    renderer_set_color *SetColor;
    renderer_draw_line *DrawLine;
    renderer_clear     *Clear;
    renderer_blit      *Blit;
    int                 res_x;
    int                 res_y;
};

struct GameAssets
{
    GameTexture bg;
    GameTexture dude;
};

typedef GameTexture load_texture(const char *asset_name);
struct GameMemory
{
    bool          is_initialized;
    float         frame_time;
    float         elapsed_time;
    load_texture *LoadTexture;
    GameRenderer  renderer;
    GameAssets    assets;
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

void GameUpdateAndRender(GameMemory &memory, GameInput &input);

#endif
