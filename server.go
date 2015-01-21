package main

import (
    ."fmt"
    "net"
    "time"
)

// Do we need multiple ports to transmit world updates?
// Or can we just send everything over 27050?
// Sending updates must be done towards each connection
// A good use case for go threads!
// The threads might timeout, in which case we drop the client

// struct ClientState
// {
//     talk // words they wish to send to world
//     has_spoken // whether we have sent these words yet
// };

// type Client struct {
//     address net.UDPAddr
// }

func SendTest() {
    socket, err := net.Dial("udp", "127.0.0.1:20750")
    defer socket.Close()
    if err != nil {
        Println("Failed to create UDP socket")
    }

    for {
        time.Sleep(1*time.Second)
        
        sent_bytes, err := socket.Write([]byte("HELLO WORLD\n"))
        if err != nil {
            Println("Failed to send data")
        }
        Printf("Sent %d bytes\n", sent_bytes)
    }


}

func main() {
    const SV_LISTEN_ADDRESS = "127.0.0.1:65387"

    address, err := net.ResolveUDPAddr("udp", SV_LISTEN_ADDRESS)
    if err != nil {
        Println("Failed to resolve UDP address")
    }

    socket, err := net.ListenUDP("udp", address)
    defer socket.Close()
    if err != nil {
        Println("Failed establish UDP listening thingy?")
    }

    // go SendTest()

    var buf []byte = make([]byte, 1024)
    for {
        read_bytes, sender, err := socket.ReadFromUDP(buf)
        if err != nil {
            Println("Something went wrong while reading from UDP")
            break
        }
        Println("Read", read_bytes, "from", sender)
    }

    // for {
    //     select {
    //     case <-client_recv:
    //         // Received data from client
    //         // Update client state

    //     case <-world_update:
    //         // Update world and send changes to all connections
    //         // Includes transmitting text
    //     }
    // }

    // This works!
    // socket, err := net.Dial("udp4", "127.0.0.1:12345")
    // defer socket.Close()
    // if err != nil {
    //     Println("Failed to create UDP socket")
    // }

    // Println("Got local address", socket.LocalAddr())

    // for {
    //     time.Sleep(1*time.Second)
        
    //     sent_bytes, err := socket.Write([]byte("HELLO WORLD\n"))
    //     if err != nil {
    //         Println("Failed to send data")
    //     }
    //     Printf("Sent %d bytes\n", sent_bytes)
    // }
}