#ifndef _server_h_
#define _server_h_
#include "SDL.h"
#include "game.h"
#include "packets.h"
#include "net.h"

void
Server(int listen_port);

#endif
