#include "net.h"
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib")

static uint32 g_socket = 0;
static bool   g_initialized = 0;
static uint16 g_preferred_port = 0;
static uint16 g_default_port = 27050;

// Debug statistics
static NetStats g_stats;
static uint64   g_last_stats_update;
static float    g_stats_update_period = 0.5f;
static int      g_bytes_sent;
static int      g_bytes_read;

NetAddress
NetMakeAddress(uint8 ip0, uint8 ip1,
uint8 ip2, uint8 ip3, uint16 port)
{
    NetAddress result = {};
    result.ip0 = ip0;
    result.ip1 = ip1;
    result.ip2 = ip2;
    result.ip3 = ip3;
    result.port = port;
    return result;
}

void
NetSetPreferredListenPort(uint16 port)
{
    g_preferred_port = port;
}

void
NetClose()
{
    WSACleanup();
}

static bool
NetInitialize()
{
    WSADATA WsaData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != NO_ERROR)
    {
        printf("Windows failure\n");
        return false;
    }

    g_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_socket <= 0)
    {
        printf("Failed to create a socket\n");
        return false;
    }

    // Bind socket to a port
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    if (g_preferred_port)
        address.sin_port = htons(g_preferred_port);
    else
        address.sin_port = g_default_port;

    if (bind(g_socket, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
    {
        printf("Failed to bind socket\n");
        return false;
    }

    // Set port to not block when calling recvfrom
    DWORD non_blocking = 1;
    if (ioctlsocket(g_socket, FIONBIO, &non_blocking) != 0)
    {
        printf("Failed to set non-blocking\n");
        return false;
    }

    return true;
}

int
NetSend(NetAddress *destination, const char *data, uint32 data_length)
{
    if (!g_initialized)
    {
        if (NetInitialize())
        {
            g_initialized = true;
        }
        else
        {
            return 0;
        }
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(
        (destination->ip0 << 24) |
        (destination->ip1 << 16) |
        (destination->ip2 <<  8) |
        (destination->ip3));
    address.sin_port = htons(destination->port);

    int bytes_sent = sendto(g_socket, data, data_length,
        0, (sockaddr*)&address, sizeof(sockaddr_in));
    g_bytes_sent += bytes_sent;
    return bytes_sent;
}

int
NetRead(char *data, uint32 max_packet_size, NetAddress *sender)
{
    if (!g_initialized)
    {
        if (NetInitialize())
        {
            g_initialized = true;
        }
        else
        {
            return 0;
        }
    }

    sockaddr_in from;
    int from_length = sizeof(from);
    int bytes_read = recvfrom(
        g_socket, data, max_packet_size, 0, (sockaddr*)&from, &from_length);
    if (bytes_read <= 0)
        return 0;
    g_bytes_read += bytes_read;

    uint32 from_address = ntohl(from.sin_addr.s_addr);
    if (sender)
    {
        sender->ip0 = (from_address >> 24) & 0xff;
        sender->ip1 = (from_address >> 16) & 0xff;
        sender->ip2 = (from_address >>  8) & 0xff;
        sender->ip3 = (from_address >>  0) & 0xff;
        sender->port = ntohs(from.sin_port);
    }
    return bytes_read;
}

NetStats
NetGetStats()
{
    if (g_last_stats_update == 0)
    {
        g_last_stats_update = GetTick() - 1;
    }

    if (TimeSince(g_last_stats_update) >
        g_stats_update_period)
    {
        g_stats.avg_bytes_sent = g_bytes_sent / g_stats_update_period;
        g_stats.avg_bytes_read = g_bytes_read / g_stats_update_period;
        g_last_stats_update = GetTick();
        g_bytes_sent = 0;
        g_bytes_read = 0;
    }

    return g_stats;
}

// High-level interface

struct GameInputPacket
{
    uint8 bits;
};

struct GamePlayerPacket
{
    float x;
    float y;
    bool connected;
};

struct GameStatePacket
{
    GamePlayerPacket players[MAX_PLAYER_COUNT];
    uint8 player_count;
};

struct ServerUpdatePacket
{
    PlayerNum player_num;
    Protocol protocol;
    Sequence sequence;
    GameStatePacket state;
    GameInputPacket inputs[MAX_PLAYER_COUNT];
};

// WriteInt, ReadInt, WriteF32, ReadF32...

void ReadPlayer(GamePlayerPacket &packet, GamePlayer &player)
{
    player.x = packet.x;
    player.y = packet.y;
    player.connected = packet.connected;
}

void WritePlayer(GamePlayer &player, GamePlayerPacket &packet)
{
    // TODO: Compression and stuff, when we get real variables here
    packet.x = player.x;
    packet.y = player.y;
    packet.connected = player.connected;
}

void ReadState(GameStatePacket &packet, GameState &state)
{
    state.player_count = int(packet.player_count);
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        ReadPlayer(packet.players[i], state.players[i]);
}

void WriteState(GameState &state, GameStatePacket &packet)
{
    packet.player_count = uint8(state.player_count);
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        WritePlayer(state.players[i], packet.players[i]);
}

void ReadInput(GameInputPacket &packet, GameInput &input)
{
    input.action1.is_down = ((1 << 0) & packet.bits) != 0;
    input.action2.is_down = ((1 << 1) & packet.bits) != 0;
    input.left.is_down    = ((1 << 2) & packet.bits) != 0;
    input.right.is_down   = ((1 << 3) & packet.bits) != 0;
    input.up.is_down      = ((1 << 4) & packet.bits) != 0;
    input.down.is_down    = ((1 << 5) & packet.bits) != 0;
}

void WriteInput(GameInput &input, GameInputPacket &packet)
{
    packet.bits &= 0;
    packet.bits |= uint8(input.action1.is_down) << 0;
    packet.bits |= uint8(input.action2.is_down) << 1;
    packet.bits |= uint8(input.left.is_down)    << 2;
    packet.bits |= uint8(input.right.is_down)   << 3;
    packet.bits |= uint8(input.up.is_down)      << 4;
    packet.bits |= uint8(input.down.is_down)    << 5;
}

// void ReadInputs(count)
// void WriteInputs(count) ?

void WriteUpdate(ServerUpdate &update, ServerUpdatePacket &packet)
{
    packet.player_num = update.player_num;
    packet.protocol = update.protocol;
    packet.sequence = update.sequence;
    WriteState(update.state, packet.state);
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        WriteInput(update.inputs[i], packet.inputs[i]);
}

void ReadUpdate(ServerUpdatePacket &packet, ServerUpdate &update)
{
    update.player_num = packet.player_num;
    update.protocol = packet.protocol;
    update.sequence = packet.sequence;
    ReadState(packet.state, update.state);
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        ReadInput(packet.inputs[i], update.inputs[i]);
}

void NetSend(NetAddress &destination, ServerUpdate &update)
{
    ServerUpdatePacket packet = {};
    WriteUpdate(update, packet);
    NetSend(&destination, (const char*)&packet, sizeof(packet));
}

bool NetRead(ServerUpdate &update, NetAddress &sender)
{
    ServerUpdatePacket packet = {};
    int read_bytes = NetRead((char*)&packet, sizeof(packet), &sender);
    if (read_bytes == 0)
        return false;

    ServerUpdate result = {};
    ReadUpdate(packet, result);
    update = result;
    return true;
}

struct ClientCmdPacket
{
    Protocol protocol;
    Sequence expected;
    GameInputPacket input;
    int32 rate;
};

void WriteClientCmd(ClientCmd &cmd, ClientCmdPacket &packet)
{
    packet.protocol = cmd.protocol;
    packet.expected = cmd.expected;
    WriteInput(cmd.input, packet.input);
    packet.rate = cmd.rate;
}

void ReadClientCmd(ClientCmdPacket &packet, ClientCmd &cmd)
{
    cmd.protocol = packet.protocol;
    cmd.expected = packet.expected;
    ReadInput(packet.input, cmd.input);
    cmd.rate = packet.rate;
}

void NetSend(NetAddress &destination, ClientCmd &cmd)
{
    ClientCmdPacket packet = {};
    WriteClientCmd(cmd, packet);
    NetSend(&destination, (const char*)&packet, sizeof(packet));
}

bool NetRead(ClientCmd &cmd, NetAddress &sender)
{
    ClientCmdPacket packet = {};
    int read_bytes = NetRead((char*)&packet, sizeof(packet), &sender);
    if (read_bytes == 0)
        return false;

    ClientCmd result = {};
    ReadClientCmd(packet, result);
    cmd = result;
    return true;
}

bool
IsPacketMoreRecent(Sequence a, Sequence b)
{
    uint16 half = (1 << 15);
    return (b > a && b - a <= half) ||
           (a > b && a - b >  half);
}
