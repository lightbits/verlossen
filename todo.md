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
