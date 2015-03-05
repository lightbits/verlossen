#ifndef _packets_h_
#define _packets_h_

#define CL_CONNECT 0xDEADBEEF
#define CL_UPDATE  0xABABABAB
#define SV_ACCEPT  0xABAD1DEA
#define SV_UPDATE  0xFABFABFA

struct ClientPacket
{
    int32 protocol;
    char  data[256];
};

struct ServerPacket
{
    int32 protocol;
    char  data[1024];
};

#endif
