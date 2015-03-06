#ifndef _packets_h_
#define _packets_h_
#include "game.h"

#define CL_CONNECT 0xDEADBEEF
#define CL_UPDATE  0xABABABAB
#define SV_ACCEPT  0xABAD1DEA
#define SV_UPDATE  0xFABFABFA
#define SV_REJECT  0xBADBADBA
#define CL_LOGOUT  0xAAAABBBB

struct ClientPacket
{
    int32 protocol;
    int32 rate;
    GameInput input;
};

struct ServerPacket
{
    int32 protocol;
    GameState state;
};

#endif
