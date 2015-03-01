#ifndef _intrinsics_h_
#define _intrinsics_h_

float
sign(float x)
{
    if (x > 0.0f) return +1.0f;
    else if (x < 0.0f) return -1.0f;
    else return 0.0f;
}

#endif
