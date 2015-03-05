
GameMemory:
    players      [MaxPlayerCount]GamePlayer
    player_count int

void
GameTick(GameMemory &memory, GameInput *inputs, int *input_map_to_player,
         int number_of_inputs)
{
    for (int i = 0; i < memory.number_of_inputs; i++)
    {
        int input_index = input_map_to_player[i];
        UpdatePlayer(memory, memory.players[i], inputs[input_index]);
    }
}

/*
not quite correct though.
*/

void
server()
{
    InitNetwork()

    world = {}
    connections = []
    for {
        inputs = []

        for select {
        case c = <-accept connection:
            connections.append(c)
        case input = <-new_input
        case <-time_to_step:
            simulate
        }
    }
}
