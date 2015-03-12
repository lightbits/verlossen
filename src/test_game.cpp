#include "test_game.h"

void
InitGameState(GameState &state)
{
    state.x = 1;
    state.y = 2;
}

void
GameTick(GameState &state, GameInput *inputs, int input_count)
{
    for (int i = 0; i < input_count; i++)
    {
        if (inputs[i].action1.is_down)
            state.x++;
        else if (inputs[i].action2.is_down)
            state.y++;
    }
}