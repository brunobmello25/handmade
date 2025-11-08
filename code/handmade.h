#include <cstddef>
#ifndef HANDMADE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define assert(expression)                                                     \
	if (!(expression)) {                                                       \
		*(int *)0 = 0;                                                         \
	}

#define arraylength(array) (sizeof(array) / sizeof((array)[0]))

#define Kilobytes(x) ((x) * (size_t)1024)
#define Megabytes(x) (Kilobytes(x) * (size_t)1024)
#define Gigabytes(x) (Megabytes(x) * (size_t)1024)
#define Terabytes(x) (Gigabytes(x) * (size_t)1024)

#define MAX_CONTROLLERS 4

#define global_variable static
#define local_persist static

// TODO(bruno): define type aliases like uint64 and int64
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
	bool isConnected;

	real32 stickAverageX;
	real32 stickAverageY;

	union {
		GameButtonState buttons[12];
		struct { // TODO(bruno): maybe deanon this struct so that we can assert
				 // that the size of buttons matches the size of the struct, at
				 // the cost of having to access stuff like
				 // controller.button.moveUp instead of controller.moveUp
			GameButtonState moveUp;
			GameButtonState moveDown;
			GameButtonState moveLeft;
			GameButtonState moveRight;

			GameButtonState actionUp;
			GameButtonState actionDown;
			GameButtonState actionLeft;
			GameButtonState actionRight;

			GameButtonState leftShoulder;
			GameButtonState rightShoulder;

			GameButtonState back;
			GameButtonState start;
		};
	};
};

struct GameInput {
	GameControllerInput controllers[MAX_CONTROLLERS + 1];
};

struct GameState {
	int toneHz;
	int xOffset;
	int yOffset;
};

inline GameControllerInput *gameGetController(GameInput *input, size_t index) {
	assert(index >= 0 && index < arraylength(input->controllers));

	GameControllerInput *controller = &input->controllers[index];

	// TODO(bruno): better assert here
	assert(&controller->start - &controller->buttons[0] ==
		   arraylength(controller->buttons) - 1);

	return controller;
}

inline u_int32_t safeTruncateUint64(u_int64_t value) {
	assert(value <=
		   0xFFFFFFFF); // TODO(bruno): defines for max values like u_int32_max
	u_int32_t result = (u_int32_t)value;
	return result;
}

//  NOTE(bruno): services that the platform layer provides to the game
//  -----------------------------------------------------------------
//  -----------------------------------------------------------------

#if HANDMADE_INTERNAL
struct DEBUGReadFileResult {
	size_t size;
	void *data;
};

DEBUGReadFileResult DEBUGPlatformReadEntireFile(const char *filename);
void DEBUGPlatformFreeFileMemory(void *memory);
bool DEBUGPlatformWriteEntireFile(const char *filename, u_int32_t size,
								  void *memory);
#endif

#define HANDMADE_H
#endif // HANDMADE_H
