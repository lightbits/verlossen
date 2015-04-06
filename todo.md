Todo
====

[] Seperate application into server and client
   Should server be a different app?
   Maybe write it in Go? Or Python!
[] Implement list of connections on server
[] Implement sending user input over several
   frames in same network packet.
[] Implement udp sockets on Linux
[] Test on Linux
[] Test over real network
[] Proper animation
[] Tilemaps? Or just handdraw the bg?

UPnP and NAT traversal
----------------------
Since most players are hidden behind a NAT router, they cannot be
hosts of a game unless they perform a NAT port mapping. The router
has a table which maps from external (ip, port) to local (ip, port).

For example, your local machine has the IP address 10.11.12.13
(it is inside a NAT), and it is running an app which listens to port
31415. Outside users cannot connect to (10.11.12.13, 31415), since
that ip is not unique for all addresses in the internet. But you can
tell your router that you wish to map incoming messages with some
destination port of your choice, to be relayed to your local machine.

I.e. create a map entry which relays messages to the router with destination
port 20000 to your machine at port 31415.

* http://blogs.msdn.com/b/ncl/archive/2009/07/27/end-to-end-connectivity-with-nat-traversal-.aspx
* http://www.codeproject.com/Articles/807861/Open-NAT-A-NAT-Traversal-library-for-NET-and-Mono#Deleteing portmappings

Network
-------
* Designated world master / host /server of game.
    - Can start application with -host argument, and
      which port to listen for connections on.
    - The host needs keep a list of connected players.
    - The host simulates world based on all user inputs.
    - Broadcasts snapshots 20 times per second (or higher)
      to each connected player in the list.

* Need to queue user inputs that are to be sent across the
  network. This is necessary when sampling less often than
  the framerate and perhaps the player has very high apm.

  When the server received a list of user inputs from a
  client, it should apply them in successive simulation
  ticks.

Game
----
* Make basic attack
* Make second main character (a wizard, perhaps?)
* Make enemy type

Resources
---------

* http://sebastiansylvan.com/2013/05/08/robin-hood-hashing-should-be-your-default-hash-table-implementation/
* https://developer.valvesoftware.com/wiki/Networking_Entities
* https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization
* https://developer.valvesoftware.com/wiki/Interpolation
* https://developer.valvesoftware.com/wiki/Prediction
* https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
* https://github.com/fogleman/Craft/tree/master/src
* http://gafferongames.com/networking-for-game-programmers/reliability-and-flow-control/
* http://gafferongames.com/networking-for-game-programmers/what-every-programmer-needs-to-know-about-game-networking/

* http://www.gdcvault.com/play/1014345/I-Shot-You-First-Networking
* http://fabiensanglard.net/quake3/network.php
