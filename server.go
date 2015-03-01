/*
http://stackoverflow.com/questions/12854125/go-how-do-i-dump-the-struct-into-the-byte-array-without-reflection
http://stackoverflow.com/questions/16330490/in-go-how-can-i-convert-a-struct-to-a-byte-array
*/

package main

import (
    "log"
    "net"
    "bytes"
    "encoding/binary"
    "time"
)

// TODO: Import these symbols from C header files
const APP_PROTOCOL   = 0xDEADBEEF
const APP_CONNECT    = 0x00000001
const APP_DISCONNECT = 0x00000002
const APP_PACKET_SIZE = 64
const APP_PROTOCOL_SIZE = 4
const APP_APP_SIZE = 4
const APP_HEADER_SIZE = APP_PROTOCOL_SIZE + APP_APP_SIZE
const APP_HAIL_SIZE = APP_PACKET_SIZE - APP_HEADER_SIZE

const SV_PACKET_SIZE = 64
const SV_LISTEN_ADDRESS = "127.0.0.1:65387"
const SV_CLIENT_RETRY_INTERVAL = 1 * time.Second
const SV_CLIENT_MAX_RETRIES = 10

type Connection struct {
    Address  string
    Socket   net.UDPConn
    // TimedOut bool
    // LastPacketReceived time
}

/*
Instead of the server requiring an explicit ACK from the client after
sending data, I'll go for a scheme where the client just continually
pushes data to the server. 

Since this is a gaming application, clients are probably going to send
data quite often. It seems reasonable that we can include a pinging
operation as well. That is, have the client send a "Hey, I'm alive!"
to the server every once in a while. If it goes too long before the 
server receives such a message, the client times out and is dropped.
*/

var SV_CONNECTIONS map[string]Connection

func SendToClient(who Connection, data []byte) {
    who.Socket.Write(data)
    // sent_bytes, err := who.Socket.Write(data)
    // if err != nil {
    //     log.Println("Failed to send data to", who.Address, "with", data)
    // }
    // log.Println("Sent", sent_bytes, "to", who.Address)
}

func AcceptIncomingConnection(who string, data *bytes.Reader) {
    // Read hail message (yes, all this code is for that)
    // TODO: Find a better way to do this
    hail_bytes := make([]byte, APP_HAIL_SIZE)
    data.Read(hail_bytes)
    // _, err := data.Read(hail_bytes)
    // if err != nil {
    //     log.Println("Failed connection from", who)
    // }
    hail_length := bytes.Index(hail_bytes, []byte{0})
    hail := string(hail_bytes[:hail_length])
    log.Println(who, "wishes to connect:", hail)

    // Add client to list of active connections
    client_sock, err := net.Dial("udp", who)
    if err != nil {
        log.Println("Failed to create UDP socket for client")
    }
    connection := Connection{who, client_sock}
    SV_CONNECTIONS[who] = connection

    // Send response
    type Response struct {
        Protocol uint32
        Welcome  [APP_PACKET_SIZE - APP_PROTOCOL_SIZE]byte
    }

    response := Response{Protocol: APP_PROTOCOL}
    copy(response.Welcome[:], []byte("Server says hello!"))
    blob := new(bytes.Buffer)
    binary.Write(blob, binary.LittleEndian, response)

    SendToClient(connection, blob.Bytes())
}

func ClientRecvWorker(who Connection) {
    packet := make([]byte, APP_PACKET_SIZE)
    reader := bytes.NewReader(packet)
    for {
        read_bytes, _, _ := who.Socket.ReadFromUDP(packet)
        var protocol uint32
        var regards  uint32
        binary.Read(reader, binary.LittleEndian, &protocol)
        binary.Read(reader, binary.LittleEndian, &regards)
        switch regards {
        case APP_DISCONNECT:
            log.Println(who.Address, "disconnected")
        }
    }
}

func main() {
    address, err := net.ResolveUDPAddr("udp", SV_LISTEN_ADDRESS)
    if err != nil {
        log.Fatalln("Failed to resolve UDP address", err)
    }

    socket, err := net.ListenUDP("udp", address)
    defer socket.Close()
    if err != nil {
        log.Fatalln("Failed create UDP socket", err)
    }

    SV_CONNECTIONS = make(map[string]Connection)

    for {
        packet := make([]byte, APP_PACKET_SIZE)
        read_bytes, sender, err := socket.ReadFromUDP(packet)
        if err != nil {
            log.Println("Something went wrong while reading from UDP", err)
            continue
        }

        var protocol uint32
        var regards uint32
        reader := bytes.NewReader(packet)
        err = binary.Read(reader, binary.LittleEndian, &protocol)
        err = binary.Read(reader, binary.LittleEndian, &regards)
        if err != nil {
            log.Println("Something went wrong while parsing packet", err)
        }

        log.Println("Read", read_bytes, "from", sender, protocol)
        if protocol != APP_PROTOCOL {
            log.Println("Received a packet with an unknown protocol")
            continue
        }

        who := sender.String()

        switch regards {
        case APP_CONNECT:
            AcceptIncomingConnection(who, reader)
        case APP_DISCONNECT:
            log.Println(who, "disconnected")
        }
    }
}