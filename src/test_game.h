#ifndef _test_game_h_
#define _test_game_h_
#include "palette.h"

#define CL_CONNECT 0xDEADBEEF
#define CL_UPDATE  0xABABABAB
#define SV_ACCEPT  0xABAD1DEA
#define SV_UPDATE  0xFABFABFA
#define SV_REJECT  0xBADBADBA
#define MAX_PLAYER_COUNT 4

enum BlitFlip
{
    BlitFlipNone,
    BlitFlipHorizontal
};

struct GameTexture
{
    int  width;
    int  height;
    void *data;
};

typedef void renderer_set_color(uint32 color);
typedef void renderer_draw_line(int x0, int y0, int x1, int y1);
typedef void renderer_clear();
typedef void renderer_blit(GameTexture texture,
    int src_x, int src_y, int src_w, int src_h,
    int dst_x, int dst_y, int dst_w, int dst_h,
    BlitFlip flip);

struct GameRenderer
{
    renderer_set_color *SetColor;
    renderer_draw_line *DrawLine;
    renderer_clear     *Clear;
    renderer_blit      *Blit;
    int res_x;
    int res_y;
};

struct GameButton
{
    int32 is_down;
};

struct GameInput
{
    GameButton action1;
    GameButton action2;
    GameButton left;
    GameButton right;
    GameButton up;
    GameButton down;
};

struct GamePlayer
{
    int x;
    int y;
    bool connected;
};

struct GameState
{
    GamePlayer players[MAX_PLAYER_COUNT];
    int player_count;
};

typedef GameTexture load_texture(const char *asset_name);
struct GameMemory
{
    GameState state;
    load_texture *LoadTexture;
};

typedef uint8 PlayerNum;

PlayerNum
GameAddPlayer(GameState &state);

void
GameDropPlayer(GameState &state, PlayerNum index);

void
GameInit(GameState &state);

void
GameTick(GameState &state,
         GameInput *inputs,
         float dt);

void
GameRender(GameMemory &memory,
           GameRenderer &renderer);

#endif
