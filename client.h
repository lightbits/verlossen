#ifndef _client_h_
#define _client_h_

struct NetPlayer
{
    float position;
    float velocity;
    // etc
};

struct NetWorld
{
    
};

void ClientConnect();
void ClientRecv();
void ClientSend(char *data, int length);
void ClientDisconnect();

#endif