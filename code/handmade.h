#include <sys/types.h>
#ifndef HANDMADE_H

#include <stddef.h>
#include <stdint.h>

#define assert(expression)                                                     \
	if (!(expression)) {                                                       \
		*(int *)0 = 0;                                                         \
	}

#define Kilobytes(x) ((x) * (size_t)1024)
#define Megabytes(x) (Kilobytes(x) * (size_t)1024)
#define Gigabytes(x) (Megabytes(x) * (size_t)1024)
#define Terabytes(x) (Gigabytes(x) * (size_t)1024)

#define MAX_CONTROLLERS 4

#define global_variable static
#define local_persist static

inline u_int32_t safeTruncateUint64(u_int64_t value) {
	assert(value <=
		   0xFFFFFFFF); // TODO(bruno): defines for max values like u_int32_max
	u_int32_t result = (u_int32_t)value;
	return result;
}

typedef float real32;
typedef double real64;

struct GameMemory {
	size_t permanentStorageSize;
	void *permanentStorage;

	size_t transientStorageSize;
	void *transientStorage;

	bool isInitialized;
};

struct GameBackbuffer {
	int width;
	int height;
	int pitch;
	void *memory;
};

struct GameSoundBuffer {
	int16_t *samples;
	int sampleRate;
	int sampleCount;
};

struct GameButtonState {
	int halfTransitionCount;
	bool endedDown;
};

struct GameControllerInput {

	bool isAnalog;

	real32 startX;
	real32 startY;

	real32 endX;
	real32 endY;

	real32 minX;
	real32 minY;

	real32 maxX;
	real32 maxY;

	union {
		GameButtonState buttons[6];
		struct {
			GameButtonState up;
			GameButtonState down;
			GameButtonState left;
			GameButtonState right;
			GameButtonState leftShoulder;
			GameButtonState rightShoulder;
		};
	};
};

struct GameInput {
	GameControllerInput controllers[4];
};

struct GameState {
	int toneHz;
	int xOffset;
	int yOffset;
};

//  NOTE(bruno): services that the platform layer provides to the game
#if HANDMADE_INTERNAL
struct DEBUGReadFileResult {
	size_t size;
	void *data;
};

DEBUGReadFileResult DEBUGPlatformReadEntireFile(const char *filename);
void DEBUGPlatformFreeFileMemory(void *memory);
bool DEBUGPlatformWriteEntireFile(char *filename, u_int32_t size, void *memory);
#endif

#define HANDMADE_H
#endif // HANDMADE_H
