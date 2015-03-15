#ifndef _platform_h_
#define _platform_h_
#include <stdint.h>
typedef uint32_t    uint;
typedef uint64_t    uint64;
typedef uint32_t    uint32;
typedef uint16_t    uint16;
typedef uint8_t     uint8;
typedef int32_t     int32;
typedef int16_t     int16;
typedef int8_t      int8;

#define Assert(expression) if(!(expression)) {*(int *)0 = 0;}

uint64 GetTick();
float GetElapsedTime(uint64 begin, uint64 end);
float TimeSince(uint64 then);

#endif
