#ifndef NET_H
#define NET_H
#include "platform.h"
#include "test_game.h"

struct NetAddress
{
    union
    {
        struct
        {
            uint8 ip0, ip1, ip2, ip3; // IP address in form ip0.ip1.ip2.ip3
        };
        uint32 ip_bytes;
    };
    uint16 port; // Which port the client is listening to
};

NetAddress NetMakeAddress(uint8 ip0, uint8 ip1,
    uint8 ip2, uint8 ip3, uint16 port);

void NetSetPreferredListenPort(uint16 port);
int  NetSend(NetAddress *destination, const char *data, uint32 data_length);
int  NetRead(char *data, uint32 max_packet_size, NetAddress *sender);
void NetClose();

// High-level interface
typedef uint16 Sequence;
typedef uint32 Protocol;
bool IsPacketMoreRecent(
    Sequence recent_than_this,
    Sequence is_this_more);

struct ServerUpdate
{
    Protocol protocol;
    Sequence sequence;
    GameState state;
    GameInput inputs[MAX_PLAYER_COUNT];
};

struct ClientCmd
{
    Protocol protocol;
    Sequence expected;
    GameInput input;
    int rate;
};

void NetSend(NetAddress &destination, ServerUpdate &update);
bool NetRead(ServerUpdate &update, NetAddress &sender);
void NetSend(NetAddress &destination, ClientCmd &cmd);
bool NetRead(ClientCmd &cmd, NetAddress &sender);

// Debugging tools
struct NetStats
{
    float avg_bytes_sent;
    float avg_bytes_read;
};
NetStats NetGetStats();

#endif
