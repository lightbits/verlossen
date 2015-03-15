#ifndef _test_game_h_
#define _test_game_h_

#define CL_CONNECT 0xDEADBEEF
#define CL_UPDATE  0xABABABAB
#define SV_ACCEPT  0xABAD1DEA
#define SV_UPDATE  0xFABFABFA
#define SV_REJECT  0xBADBADBA
typedef uint8 PlayerNum;

#define MAX_PLAYER_COUNT 4

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

PlayerNum GameAddPlayer(GameState &state);
void GameDropPlayer(GameState &state, PlayerNum index);
void GameInit(GameState &state);

// The input at a given index must correspond to the
// player with the same PlayerNum
void GameTick(GameState &state,
              GameInput inputs[MAX_PLAYER_COUNT]);

#endif
