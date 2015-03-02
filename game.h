#ifndef _game_h_
#define _game_h_
#define MaxPlayerCount 2
#include "matrix.h"
#include "platform.h"

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
};

struct GameState
{
    GamePlayer players[MaxPlayerCount];
    int next_player_index;

    float tile_side_in_meters;
    float tile_side_in_pixels;
    float pixels_per_meter;
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
    int res_x;
    int res_y;
};

struct GameAssets
{
    GameTexture bg;
    GameTexture hero;
};

struct GameButton
{
    bool is_down;
};

struct GameInput
{
    float frame_time;
    float elapsed_time;
    GameButton action1;
    GameButton action2;
    GameButton left;
    GameButton right;
    GameButton up;
    GameButton down;
};

struct GameNetworkPacket
{
    vec2 position;
    GameInput input;
};

struct GameNetworkData
{
    // TODO: What the heck is this even
    bool fresh_update;
    GameNetworkPacket incoming;
    GameNetworkPacket outgoing;
};

typedef GameTexture load_texture(const char *asset_name);
struct GameMemory
{
    bool is_initialized;
    GameAssets assets;
    GameState state;
    GameNetworkData network;

    // Debug functions
    load_texture *LoadTexture;
};

void
GameUpdateAndRender(GameMemory &memory,
                    GameRenderer &renderer,
                    GameInput &input);

#endif
