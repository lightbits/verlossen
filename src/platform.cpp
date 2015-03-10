#include "platform.h"

uint64
GetTick()
{
    return SDL_GetPerformanceCounter();
}

float
GetElapsedTime(uint64 begin, uint64 end)
{
    uint64 frequency = SDL_GetPerformanceFrequency();
    return (float)(end - begin) / (float)frequency;
}

float
TimeSince(uint64 then)
{
    uint64 now = GetTick();
    return GetElapsedTime(then, now);
}
