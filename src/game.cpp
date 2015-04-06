#include "game.h"
#include "palette.h"
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}

void
DrawSprite(GameRenderer &render, GameTexture &texture, GameSprite &sprite, int dst_x, int dst_y)
{
    int framekey = sprite.current_frame +
                   sprite.current_animation * sprite.frame_count;
    int src_x = sprite.src_x[framekey];
    int src_y = sprite.src_y[framekey];
    BlitFlip flip = sprite.flipped ? BlitFlipHorizontal : BlitFlipNone;
    render.Blit(texture,
        src_x, src_y, sprite.src_w, sprite.src_h,
        dst_x, dst_y, sprite.src_w, sprite.src_h,
        flip);
}

void
DrawTexture(GameRenderer &render, GameTexture texture, int x, int y)
{
    render.Blit(texture,
        0, 0, texture.width, texture.height,
        x, y, texture.width, texture.height,
        BlitFlipNone);
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

GameSprite
make_sprite(float frame_delay,
           int frame_count, int anim_count,
           int *src_x,      int *src_y,
           int src_w,       int src_h)
{
    GameSprite result = {};
    result.current_frame = 0;
    result.frame_delay = frame_delay;
    result.frame_timer = frame_delay;
    result.frame_count = frame_count;
    result.anim_count  = anim_count;
    for (int i = 0; i < frame_count * anim_count; i++)
    {
        result.src_x[i] = src_x[i];
        result.src_y[i] = src_y[i];
    }
    result.src_w = src_w;
    result.src_h = src_h;
    return result;
}

void
GamePushPlayer(GameMemory &memory)
{
    assert(memory.state.player_count + 1 < MAX_PLAYERS);
    GamePlayer player        = {};
    player.position          = Vec2(2.0f, 2.0f);
    player.velocity          = Vec2(0.0f, 0.0f);
    player.size              = Vec2(0.8f, 1.5f);
    player.run_acceleration  = 22.0f;
    player.jump_timer        = 0.0f;
    player.jump_duration     = 0.05f;
    player.jump_acceleration = 100.0f;
    player.sprite            = memory.assets.spr_hero;
    memory.state.players[memory.state.player_count++] = player;
}

void
GameRemovePlayer(GameMemory &memory, int i)
{
    for (; i < memory.state.player_count; i++)
    {
        memory.state.players[i] = memory.state.players[i + 1];
    }
    memory.state.player_count--;
}


void
GameLoadTextures(GameMemory &memory)
{
    memory.assets.tex_bgnd = memory.LoadTexture("plains.bmp");
    memory.assets.tex_hero = memory.LoadTexture("hero_sheet.bmp");
}

void
GameInit(GameMemory &memory)
{
    int src_x[6] = {0, 32,  0, 32, 32, 32};
    int src_y[6] = {0,  0, 32, 32, 32, 32};
    memory.assets.spr_hero = make_sprite(
        0.35f, 2, 3, src_x, src_y, 32, 32);

    // Initialize parameters
    memory.state.tile_side_in_meters = 1.00f;
    memory.state.tile_side_in_pixels = 16.0f;
    memory.state.pixels_per_meter = memory.state.tile_side_in_pixels /
                                    memory.state.tile_side_in_meters;
}

void
AnimateSprite(GameSprite &sprite, float dt)
{
    sprite.frame_timer -= dt;
    if (sprite.frame_timer <= 0.0f)
    {
        sprite.current_frame++;
        if (sprite.current_frame >= sprite.frame_count)
            sprite.current_frame = 0;
        sprite.frame_timer = sprite.frame_delay;
    }
}

void
UpdatePlayer(GameInput &input, GameState &state, GamePlayer &player, float dt)
{
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

                // TODO: player.sprite.frame_timer = 0.0f;
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
    bool moving_left = false;
    bool moving_right = false;
    if (input.left.is_down)
    {
        moving_left = true;
        player.acceleration.x = -player.run_acceleration;
    }
    else if (input.right.is_down)
    {
        moving_right = true;
        player.acceleration.x = +player.run_acceleration;
    }

    float friction = player.velocity.x * 7.0f;
    player.acceleration.x -= friction;

    player.velocity += player.acceleration * dt;
    player.position += player.velocity * dt +
                       player.acceleration * dt * dt * 0.5f;

    // Animation
    // TODO: Figure out how to merge this with above state machine
    if (player.state == PLAYER_JMP1 ||
        player.state == PLAYER_JMP2 ||
        player.state == PLAYER_JMP3)
    {
        player.sprite.current_animation = 2;
    }
    else if (moving_left || moving_right)
    {
        player.sprite.current_animation = 1;
    }
    else if (moving_right)
    {
        player.sprite.current_animation = 1;
    }
    else
    {
        player.sprite.current_animation = 0;
    }

    if (moving_left)
        player.sprite.flipped = true;
    else if (moving_right)
        player.sprite.flipped = false;

    AnimateSprite(player.sprite, dt);
}

void
GameUpdate(GameMemory &memory,
           GameInput *inputs,
           float dt)
{
    for (int i = 0; i < memory.state.player_count; i++)
    {
        UpdatePlayer(
            inputs[i], memory.state,
            memory.state.players[i], dt);
    }
}

void
GameRender(GameMemory &memory, GameRenderer &render)
{
    GameAssets *assets = &memory.assets;
    render.SetColor(PAL16_VOID);
    render.Clear();
    DrawTexture(render, assets->tex_bgnd, 0, 0);

    for (int i = 0; i < memory.state.player_count; i++)
    {
        GamePlayer *player = &memory.state.players[i];
        int x = player->position.x * memory.state.pixels_per_meter -
                player->sprite.src_w / 2;
        int y = render.res_y -
                player->position.y * memory.state.pixels_per_meter -
                player->sprite.src_h;
        DrawSprite(render, assets->tex_hero, player->sprite, x, y);

        // int px = player->position.x * memory.state.pixels_per_meter;
        // int py = render.res_y - player->position.y * memory.state.pixels_per_meter;
        // int w = player->size.x * memory.state.pixels_per_meter;
        // int h = player->size.y * memory.state.pixels_per_meter;
        // DrawDebugRectangle(render, px - w / 2, py - h, w, h);
    }
}
