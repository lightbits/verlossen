Verlossen
=========

Play with your friend across the world using state-of-the-art internet technology, and defend the plainlands against the incoming hoard of hungry boars.

##Development log:##

###Day 4 (March 4. 2015)###


###Day 3 (March 3. 2015)###
Spent some time making a running animation and a sort of idle animation. The game (if I can call it that) looks alot more spenstig now - as we say in Norway.

![](data/hero_run.gif)
![](data/hero_stand.gif)
![](data/ingame_run.gif)



Regarding the network code: I'm thinking I'll need to make one of the players a "host" or "master" sort of thing. My argument is that if the game will have enemy entities that both players can interact with, then someone needs to simulate those enemies.

* One approach is to let both players have a list of active enemies, that they simulate locally and share with the other player. But this requires some messy synchronization, and I really don't want to do that.

* A - probably cleaner - approach, is to make one of the players a dedicated world master. The world master will simulate the world, given both player inputs, and send the result back.

I suppose the world master can be both a remote server, or something that runs locally on one of the player's machines. But I probably want to have the player that creates a new game be the master, and the joinee listens. To make things appear smooth, I suspect I'll need to simulate (predict?) the world state locally as well, and then blend in the actual world state when it arrives over the network.

* **Compile time**: 1.9 seconds (!)
* **Art assets**: 3
* **Source files**: 9

###Day 2 (March 2. 2015)###

*Aw yiss*. Got networking going. Somewhat. It's really janky and needs more work. To be specific, I'll need to describe how it works so far. I am currently sending both player position and input over the network. When I receive updates from someone else, I add their character if not added already.

On every consequent update, I update the second player's position whenever I get a new network packet, and simulate with the input until the next packet. This results in ugly jitter due to delays across the web, so I should rather use some interpolation scheme to smoothly blend between simulated state and measured state. More that later!

* **Compile time**: 1.6 seconds
* **Art assets**: 2
* **Source files**: 9
* **Player count**: 2

![](data/preview_02.png)

Also, note the slight art modification. Our hero is no longer a vampire! (What is he though, a bard?)

###Day 1 (March 1. 2015)###

Made character and scenery artwork. This might be the best art that my hands have ever produced. I use Arne's 16-color palette, which is amazing and delicious and you should use it too.

* **Compile time**: 1.4 seconds
* **Art assets**: 2
* **Source files**: 9

![](data/preview_01.png)

###Day ?? (??)###
Wrote the machinery that will hold this thing together. Compile times are fast, which is a good sign. Wrote some rudimentary socket code, but only for windows. I suspect that socket programming is pretty similiar on linux as well though, so porting will be a breeze.

* **Compile time**: 1.2 seconds
* **Source files**: 9
