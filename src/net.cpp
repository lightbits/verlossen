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

void NetSetPreferredListenPort(uint16 port)
{
    g_preferred_port = port;
}

void NetClose()
{
    WSACleanup();
}

static
bool NetInitialize()
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

int NetSend(NetAddress *destination, const char *data, uint32 data_length)
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

int NetRead(char *data, uint32 max_packet_size, NetAddress *sender)
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

NetStats NetGetStats()
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
