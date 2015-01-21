#include <stdio.h>
#include "net.h"
#include "win32_net.cpp"
#include <windows.h>

int   CL_LISTEN_PORT = 12345;
int   SV_TICKRATE = 20;
int   CL_CMDRATE = 20;
int   CL_UPDATERATE = 20;
int   SV_LISTEN_PORT = 65387;
int   SV_MAX_CONNECTIONS = 8;
int   SV_TIMEOUT_SECONDS = 10;
float CL_INTERP = 0.01f;

struct ConnectionResponse
{
    uint32 welcome_length;
    char welcome[255];
};

struct ConnectionRequest
{
    uint32 protocol;
};

ConnectionResponse
ClientConnect(NetAddress *server)
{
    ConnectionRequest request = {};
    request.protocol = 0xdeadbeef;
    NetAddress dst = { 127, 0, 0, 1, SV_LISTEN_PORT };

    while (1)
    {
        NetSend(&dst, (const char*)&request, sizeof(ConnectionRequest));
        Sleep(1000);
    }

    while (!NetSend(&dst, (const char*)&request, sizeof(ConnectionRequest)))
    {
        // Wait...
    }

    // Ok now wait for response
    ConnectionResponse response = {};
    // uint32 read_bytes = NetRead(
    //     (uint8*)&response, sizeof(ConnectionResponse), server);
    // while (read_bytes < sizeof(ConnectionResponse))
    // {
    //     read_bytes = NetRead(
    //         (uint8*)&response, sizeof(ConnectionResponse), server);
    // }
    return response;
}

void
ClientDisconnect(NetAddress *server)
{

}

int 
main(int argc, char **argv)
{
    // This works!
    NetSetPreferredListenPort(CL_LISTEN_PORT);
    const char *data = "HELLO WORLD";
    NetAddress dst = { 127, 0, 0, 1, SV_LISTEN_PORT };
    while (1)
    {
        int sent_bytes = NetSend(&dst, data, 12);
        printf("Sent %d bytes\n", sent_bytes);
        Sleep(1000);
    }
    ////////////////////////////////////////////////

    // This works!
    // NetSetPreferredListenPort(12345);
    // NetAddress sender = {};
    // char data[256];
    // int bytes_read = NetRead(data, 256, &sender);
    // while (bytes_read <= 0)
    // {
    //     bytes_read = NetRead(data, 256, &sender);
    // }
    // printf("Read %d bytes\n", bytes_read);
    // Sleep(5000);
    ////////////////////////////////////////////////

    // NetAddress server = {};
    // NetSetPreferredListenPort(CL_LISTEN_PORT);
    // ConnectionResponse response = ClientConnect(&server);
    // printf("Connection established\n");
    // printf("sv port: %d\n", server.port);
    // printf("welcome: %s\n", response.welcome);
    // ClientDisconnect(&server);
    NetClose();
    return 0;
}