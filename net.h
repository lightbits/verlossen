#ifndef NET_H
#define NET_H
#include "platform.h"

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