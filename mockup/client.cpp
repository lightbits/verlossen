


/*
TODO: Move networking stuff out of GameUpdateAndRender
It will be cleaner if it just gets a single world state,
and we extract the information that we need to send from
the Client loop.
*/

void
Client()
{
    InitSDL()
    InitRenderer()
    InitGame()
    ConnectToServer(...)

    predicted_world = {}

    while running {
        HandleInput(input)

        sv_update = Recv() // This extracts one element from
                           // the ring buffer, if there is any.
        if (sv_update) {
            predicted_world = World(sv_update.world_state)
            while (predicted_world.tick != local_tick) {
                predicted_world = StepWorld([sv_update.inputs, input])
            }
        }

        GameUpdateAndRender(world, input)
    }
}
