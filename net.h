#ifndef NET_H
#define NET_H
#include <stdint.h>
typedef uint32_t    uint;
typedef uint64_t    uint64;
typedef uint32_t    uint32;
typedef uint16_t    uint16;
typedef uint8_t     uint8;
typedef int32_t     int32;
typedef int16_t     int16;
typedef int8_t      int8;

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

void NetSetPreferredListenPort(uint16 port);
int  NetSend(NetAddress *destination, const char *data, uint32 data_length);
int  NetRead(char *data, uint32 max_packet_size, NetAddress *sender);
void NetClose();

#endif