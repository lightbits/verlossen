#include "game.h"
#include "palette.h"
#include "intrinsics.h"
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}

void
DrawSprite(GameRenderer &render, GameTexture texture, int x, int y)
{
    render.Blit(texture,
        0, 0, texture.width, texture.height,
        x, y, texture.width, texture.height);
}

void
DrawDebugRectangle(GameRenderer &render, int x, int y, int w, int h)
{
    render.SetColor(PAL16_BLOODRED);
    render.DrawLine(x, y, x + w, y);
    render.DrawLine(x + w, y, x + w, y + h);
    render.DrawLine(x, y, x, y + h);
    render.DrawLine(x, y + h, x + w, y + h);
}

void
LoadAssets(GameMemory &memory)
{
    memory.assets.bg   = memory.LoadTexture("plains.bmp");
    memory.assets.hero = memory.LoadTexture("dude_nbp.bmp");
}

void
UpdatePlayer(GameInput &input, GameState &state, GamePlayer &player)
{
    float dt = input.frame_time;
    bool jump_key_down  = input.action1.is_down;
    bool jump_key_up    = !input.action1.is_down;
    bool jump_timeout   = (player.jump_timer >= player.jump_duration) &&
                         (player.jump_timer - dt <= player.jump_duration);
    float ground_height = 1 * state.tile_side_in_meters;
    bool hit_ground     = player.position.y +
                         player.velocity.y * dt <= ground_height;
    bool lost_ground    = player.position.y > ground_height;

    float gravity = -9.81f;
    player.acceleration.y = gravity;

    switch (player.state)
    {
        case PLAYER_INIT:
        {
            if (lost_ground)
            {
                player.state = PLAYER_FALLING;
            }
            else
            {
                player.state = PLAYER_IDLE;
            }
        } break;
        case PLAYER_IDLE:
        {
            player.acceleration.y = 0.0f;
            player.velocity.y = 0.0f;
            if (jump_key_down)
            {
                player.state = PLAYER_JMP1;
                player.jump_timer = 0.0f;
            }
            else if (lost_ground)
            {
                player.state = PLAYER_FALLING;
            }
        } break;
        case PLAYER_JMP1:
        {
            player.acceleration.y = gravity + player.jump_acceleration;
            player.jump_timer += dt;
            if (jump_key_up)
            {
                player.state = PLAYER_JMP3;
            }
            else if (jump_timeout)
            {
                player.state = PLAYER_JMP2;
            }
        } break;
        case PLAYER_JMP2:
        {
            player.acceleration.y = gravity;
            if (jump_key_up)
            {
                player.state = PLAYER_JMP3;
            }
            else if (hit_ground)
            {
                player.state = PLAYER_IDLE_JMP;
                player.position.y = ground_height;
                player.velocity.y = 0.0f;
                player.acceleration.y = 0.0f;
            }
        } break;
        case PLAYER_JMP3:
        {
            player.acceleration.y = gravity;
            if (hit_ground)
            {
                player.state = PLAYER_IDLE;
                player.position.y = ground_height;
                player.velocity.y = 0.0f;
                player.acceleration.y = 0.0f;
            }
        } break;
        case PLAYER_IDLE_JMP:
        {
            player.acceleration.y = 0.0f;
            if (jump_key_up)
            {
                player.state = PLAYER_IDLE;
            }
            else if (lost_ground)
            {
                player.state = PLAYER_FALLING;
            }
        } break;
        case PLAYER_FALLING:
        {
            player.acceleration.y = gravity;
            if (hit_ground)
            {
                player.state = PLAYER_IDLE;
                player.position.y = ground_height;
                player.velocity.y = 0.0f;
                player.acceleration.y = 0.0f;
            }
        } break;
    }

    player.acceleration.x = 0.0f;
    if (input.left.is_down)
        player.acceleration.x = -player.run_acceleration;
    else if (input.right.is_down)
        player.acceleration.x = +player.run_acceleration;

    float friction = player.velocity.x * 7.0f;
    player.acceleration.x -= friction;

    player.velocity += player.acceleration * dt;
    player.position += player.velocity * dt +
                       player.acceleration * dt * dt * 0.5f;
}

void
PushPlayer(GameMemory &memory)
{
    assert(memory.state.next_player_index < MaxPlayerCount);
    GamePlayer player = {};
    player.position  = Vec2(2.0f, 2.0f);
    player.velocity  = Vec2(0.0f, 0.0f);
    player.size      = Vec2(0.8f, 1.5f);
    player.run_acceleration = 22.0f;
    player.jump_timer = 0.0f;
    player.jump_duration = 0.05f;
    player.jump_acceleration = 100.0f;
    memory.state.players[memory.state.next_player_index++] = player;
}

void
GameUpdateAndRender(GameMemory   &memory,
                    GameRenderer &render,
                    GameInput    &input)
{
    GameAssets *assets = &memory.assets;

    if (!memory.is_initialized)
    {
        LoadAssets(memory);
        PushPlayer(memory);
        memory.state.tile_side_in_meters = 1.00f;
        memory.state.tile_side_in_pixels = 16.0f;
        memory.state.pixels_per_meter = memory.state.tile_side_in_pixels /
                                        memory.state.tile_side_in_meters;
        memory.is_initialized = true;
    }

    GameNetworkPacket incoming = memory.network.incoming;
    if (incoming.input.action1.is_down)
    {
        if (memory.state.next_player_index == 1)
        {
            PushPlayer(memory);
        }
    }

    UpdatePlayer(input, memory.state, memory.state.players[0]);
    UpdatePlayer(incoming.input, memory.state, memory.state.players[1]);
    if (memory.network.fresh_update)
    {
        memory.state.players[1].position = incoming.position;
    }
    memory.network.outgoing.position = memory.state.players[0].position;
    memory.network.outgoing.input = input;

    render.SetColor(PAL16_VOID);
    render.Clear();
    DrawSprite(render, assets->bg, 0, 0);

    for (int i = 0; i < memory.state.next_player_index; i++)
    {
        GamePlayer *player = &memory.state.players[i];
        int x = player->position.x * memory.state.pixels_per_meter -
                assets->hero.width / 2;
        int y = render.res_y -
                player->position.y * memory.state.pixels_per_meter -
                assets->hero.height;
        DrawSprite(render, assets->hero, x, y);

        // int px = player->position.x * memory.state.pixels_per_meter;
        // int py = render.res_y - player->position.y * memory.state.pixels_per_meter;
        // int w = player->size.x * memory.state.pixels_per_meter;
        // int h = player->size.y * memory.state.pixels_per_meter;
        // DrawDebugRectangle(render, px - w / 2, py - h, w, h);
    }
}
