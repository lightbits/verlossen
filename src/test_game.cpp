#include "test_game.h"

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
    while (index < MAX_PLAYER_COUNT &&
           state.players[index].connected)
        index++;
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
    if (input.action1.is_down)
        player.x++;
    if (input.action2.is_down)
        player.y++;
}

void
GameTick(GameState &state, GameInput inputs[MAX_PLAYER_COUNT])
{
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (!state.players[i].connected)
            continue;
        PlayerTick(state.players[i], inputs[i]);
    }
}

bool
IsPacketMoreRecent(Sequence a, Sequence b)
{
    uint16 half = (1 << 15);
    return (b > a && b - a <= half) ||
           (a > b && a - b >  half);
}
