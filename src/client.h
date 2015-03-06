#ifndef _client_h_
#define _client_h_
#include "SDL.h"
#include "game.h"
#include "packets.h"
#include "net.h"
#include "palette.h"
#define CL_UPDATE_RATE 20

void
Client(NetAddress server_addr, int listen_port);

#endif
