#ifndef HANDMADE_INTRINSICS_H

#include "handmade.h"
// TODO(bruno): stop using cmath for these functions and implement them
#include <cmath>

inline int32 roundReal32ToInt32(real32 value) { return (int32)(value + 0.5f); }
inline uint32 roundReal32ToUInt32(real32 value) {
	return (uint32)(value + 0.5f);
}
inline int32 floorReal32ToInt32(real32 value) { return floorf(value); }
inline uint32 floorReal32ToUInt32(real32 value) { return floorf(value); }

inline real32 sin(real32 angle) { return sinf(angle); }
inline real32 cos(real32 angle) { return cosf(angle); }
inline real32 atan2(real32 y, real32 x) { return atan2f(y, x); }

#define HANDMADE_INTRINSICS_H
#endif
