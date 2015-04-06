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
    player.x = 0;
    player.y = 0;
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
PlayerTick(GamePlayer &player, GameInput &input)
{
    if (input.left.is_down)  player.x--;
    if (input.right.is_down) player.x++;
    if (input.down.is_down)  player.y--;
    if (input.up.is_down)    player.y++;
}

void
GameTick(GameState &state,
         GameInput *inputs,
         float dt)
{
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (state.players[i].connected)
            PlayerTick(state.players[i], inputs[i]);
    }
}

void
DebugDrawPlayer(GameRenderer &render,
                GamePlayer &player,
                int index)
{
    int w = 20;
    int h = 20;
    int x = player.x + w / 2;
    int y = render.res_y - player.y - h;
    uint32 color = PAL16[1 + index % 15];
    DrawDebugRectangle(render, x, y, 20, 20, color);
}

void
GameRender(GameMemory &memory,
           GameRenderer &render)
{
    render.SetColor(PAL16_VOID);
    render.Clear();
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        DebugDrawPlayer(render, memory.state.players[i], i);
    }
}
