#ifndef _test_game_h_
#define _test_game_h_

#define CL_CONNECT 0xDEADBEEF
#define CL_UPDATE  0xABABABAB
#define SV_ACCEPT  0xABAD1DEA
#define SV_UPDATE  0xFABFABFA
#define SV_REJECT  0xBADBADBA
typedef uint16 Sequence;

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

struct GameState
{
    int x;
    int y;
};

struct ServerPacket
{
    uint32 protocol;
    Sequence sequence;
    GameState state;
};

struct ClientPacket
{
    uint32 protocol;
    Sequence expected;
    GameInput input;
    int rate;
};

// TODO: Figure where this belongs. Perhaps in net module?
bool IsPacketMoreRecent(
    Sequence recent_than_this,
    Sequence is_this_more);

void InitGameState(GameState &state);

#endif
