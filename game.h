#ifndef _game_h_
#define _game_h_
#define MaxPlayerCount 2
#include "matrix.h"
#include "platform.h"

enum BlitFlip
{
    BlitFlipNone,
    BlitFlipHorizontal
};

struct GameTexture
{
    int width;
    int height;
    void *data;
};

#define MaxSpriteFrameCount 16
struct GameSprite
{
    GameTexture texture;
    int current_frame;
    int current_animation;
    float frame_delay; // Milliseconds each frame should last
    float frame_timer;
    int frame_count; // Number of frames per animation
    int anim_count; // Number of rows (of frames)
    int src_x[MaxSpriteFrameCount]; // X and Y offsets to start of frame
    int src_y[MaxSpriteFrameCount]; // inside the texture.
    int src_w;
    int src_h;
    bool flipped;
};

enum PlayerState
{
    PLAYER_INIT,
    PLAYER_IDLE,
    PLAYER_JMP1,
    PLAYER_JMP2,
    PLAYER_JMP3,
    PLAYER_IDLE_JMP,
    PLAYER_FALLING
};

struct GamePlayer
{
    vec2 position;
    vec2 velocity;
    vec2 acceleration;
    vec2 size;
    float run_acceleration;
    float jump_timer;
    float jump_duration;
    float jump_acceleration;
    PlayerState state;

    GameSprite sprite;
};

struct GameState
{
    GamePlayer players[MaxPlayerCount];
    int next_player_index;

    float tile_side_in_meters;
    float tile_side_in_pixels;
    float pixels_per_meter;
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

struct GameAssets
{
    GameTexture tex_bgnd;
    GameTexture tex_hero;
    GameSprite  spr_hero;
};

struct GameButton
{
    bool is_down;
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

typedef GameTexture load_texture(const char *asset_name);
struct GameMemory
{
    GameAssets assets;
    GameState state;

    // Debug functions
    load_texture *LoadTexture;
    int next_player_index;
};

void
GameInit(GameMemory &memory);

void
GameUpdate(GameMemory &memory,
           GameInput *inputs,
           int *input_map_player,
           int input_count,
           float dt);

void
GameRender(GameMemory &memory, GameRenderer &renderer);

#endif
