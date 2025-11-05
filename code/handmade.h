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

#define HANDMADE_H
#endif // HANDMADE_H
