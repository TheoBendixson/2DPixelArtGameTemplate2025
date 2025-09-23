#include "math.h"

internal s32 
RoundReal32ToInt32(r32 Number)
{
    s32 Result = (s32)roundf(Number);
    return Result;
}
    
inline u32 
TruncateReal32ToInt32(r32 Number)
{
    s32 Result = (s32)(Number);
    return Result;
}

inline u32 
CeilReal32ToInt32(r32 Number)
{
    s32 Result = (s32)ceilf(Number);
    return Result;
}

inline s32 
SignOf(s32 Value)
{
    s32 Result = (Value >= 0) ? 1 : -1;
    return (Result);
}

inline r32
SquareRoot(r32 Real32)
{
    r32 Result = sqrt(Real32);
    return (Result);
}

inline r32 
AbsoluteValue(r32 Real32)
{
    r32 Result = fabs(Real32);
    return (Result);
}

internal u32 
RoundReal32ToUint32(r32 Number)
{
    u32 Result = (u32)roundf(Number);
    return Result;
}

r32 
Floor(r32 X)
{
	r32 Result = floorf(X);
	return Result;
}

r32
RandomFloat(r32 Min, r32 Max)
{
    r32 RandomNumber = (r32)(rand())/RAND_MAX;
    r32 Result = Min + RandomNumber*(Max - Min);
    return Result;
}
