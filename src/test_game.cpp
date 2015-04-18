#include "test_game.h"

void
DrawDebugRectangle(GameRenderer &render, int x, int y, int w, int h,
                   uint32 color)
{
    render.SetColor(color);
    render.DrawLine(x, y, x + w, y);
    render.DrawLine(x + w, y, x + w, y + h);
    render.DrawLine(x, y, x, y + h);
    render.DrawLine(x, y + h, x + w, y + h);
}

void
GameInit(GameState &state)
{
    state.player_count = 0;

    // TODO: Render-agnostic coordinates for game objects
    // just pass NDC or whatever to draw functions here,
    // and convert to screen coords using GameRenderer
    state.map_dimensions.x = 320.0f;
    state.map_dimensions.y = 160.0f;
}

PlayerNum
GameAddPlayer(GameState &state)
{
    Assert(state.player_count < MAX_PLAYER_COUNT);
    int index = 0;
    // Find first available player slot
    while (index < MAX_PLAYER_COUNT &&
           state.players[index].connected) {
        index++;
    }
    Assert(index < MAX_PLAYER_COUNT);
    GamePlayer player = {};
    player.connected = true;

    switch (index)
    {
        case 0:
            player.position.x = state.map_dimensions.x / 8.0f;
            player.position.y = state.map_dimensions.y / 2.0f;
            break;
        case 1:
            player.position.x = state.map_dimensions.x * 7.0f / 8.0f;
            player.position.y = state.map_dimensions.y / 2.0f;
            break;
        case 2:
            player.position.x = state.map_dimensions.x / 2.0f;
            player.position.y = state.map_dimensions.y / 8.0f;
            break;
        case 3:
            player.position.x = state.map_dimensions.x / 2.0f;
            player.position.y = state.map_dimensions.y * 7.0f / 2.0f;
            break;
    }
    state.players[index] = player;
    state.player_count++;
    return PlayerNum(index);
}

void
GameDropPlayer(GameState &state, PlayerNum index)
{
    Assert(index < MAX_PLAYER_COUNT);
    state.players[index].connected = false;
    state.player_count--;
}

void
PlayerTick(GamePlayer &player, GameInput &input, float dt)
{
    // float speed = 40.0f;
    // if (input.left.is_down)  player.x -= speed * dt;
    // if (input.right.is_down) player.x += speed * dt;
    // if (input.down.is_down)  player.y -= speed * dt;
    // if (input.up.is_down)    player.y += speed * dt;
}

void
GameTick(GameState &state,
         GameInput *inputs,
         float dt)
{
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (state.players[i].connected)
            PlayerTick(state.players[i], inputs[i], dt);
    }
}

void
DebugDrawPlayer(GameRenderer &render,
                GamePlayer &player,
                uint32 color)
{
    int w = 15;
    int h = 60;
    int x = int(player.position.x) - w / 2;
    int y = render.res_y - int(player.position.y) - h / 2;
    DrawDebugRectangle(render, x, y, w, h, color);
}

void
GameRender(GameMemory &memory,
           GameRenderer &render)
{
    render.SetColor(PAL16_VOID);
    render.Clear();
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        uint32 color = PAL16[1 + i % 15];
        if (memory.state.players[i].connected)
            DebugDrawPlayer(render, memory.state.players[i], color);
    }
}
