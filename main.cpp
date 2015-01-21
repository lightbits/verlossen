#include <stdio.h>
#include "net.h"
#include "win32_net.cpp"
#include <windows.h>

const int   CL_LISTEN_PORT = 12345;
const int   SV_TICKRATE = 20;
const int   CL_CMDRATE = 20;
const int   CL_UPDATERATE = 20;
const int   CL_PACKET_SIZE = 64;
const int   SV_PACKET_SIZE = 64;
const int   SV_LISTEN_PORT = 65387;
const int   SV_LISTEN_IP0  = 127;
const int   SV_LISTEN_IP1  = 0;
const int   SV_LISTEN_IP2  = 0;
const int   SV_LISTEN_IP3  = 1;
const int   SV_MAX_CONNECTIONS = 8;
const int   SV_TIMEOUT_SECONDS = 10;
const float CL_INTERP = 0.01f;

// Message types
const uint32 APP_PROTOCOL   = 0xDEADBEEF;
const uint32 APP_CONNECT    = 0x00000001;
const uint32 APP_DISCONNECT = 0x00000002;

struct ConnectionResponse
{
    uint32 protocol;
    char welcome[SV_PACKET_SIZE - sizeof(uint32)];
};

struct Acknowledge
{
    uint32 protocol;
    uint32 regards;
};

struct ConnectionRequest
{
    uint32 protocol;
    uint32 connect;
    char   hail[CL_PACKET_SIZE - 2 * sizeof(uint32)];
};

ConnectionResponse
ClientConnect(NetAddress *server)
{
    ConnectionRequest request = {};
    request.protocol = APP_PROTOCOL;
    request.connect  = APP_CONNECT;

    // TODO: Replace with safer snprintf
    sprintf(request.hail, "Yo wassup!");
    NetAddress dst = { 
        SV_LISTEN_IP0, 
        SV_LISTEN_IP1, 
        SV_LISTEN_IP2, 
        SV_LISTEN_IP3, 
        SV_LISTEN_PORT 
    };

    ConnectionResponse response = {};
    int read_bytes = 0;
    int max_read_tries = 5;
    printf("Attempting to connect to %d.%d.%d.%d:%d\n", 
        dst.ip0, dst.ip1, dst.ip2, dst.ip3, dst.port);
    while (read_bytes <= 0)
    {
        // Send our connection request!
        NetSend(&dst, (char*)&request, sizeof(ConnectionRequest));
        
        int read_tries = 0;
        bool valid = false;
        while (read_bytes <= 0 && read_tries < max_read_tries && !valid)
        {
            read_bytes = NetRead(
                (char*)&response, sizeof(ConnectionResponse), server);

            if (response.protocol == APP_PROTOCOL)
                valid = true;



            read_tries++;
            Sleep(1000);
        }
        printf("Retrying connection\n");
    }
    return response;
}

void
ClientDisconnect(NetAddress *server)
{

}

int 
main(int argc, char **argv)
{
    NetAddress server = {};
    NetSetPreferredListenPort(CL_LISTEN_PORT);
    ConnectionResponse response = ClientConnect(&server);
    printf("Connection established\n");
    printf("sv port: %d\n", server.port);
    printf("welcome: %s\n", response.welcome);
    ClientDisconnect(&server);
    NetClose();
    Sleep(5000);
    return 0;
}