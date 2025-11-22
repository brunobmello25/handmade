#ifndef HANDMADE_H

#include <stddef.h>
#include <stdint.h>

typedef float real32;
typedef double real64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define assert(expression)                                                     \
	if (!(expression)) {                                                       \
		_Pragma("GCC diagnostic push")                                         \
				_Pragma("GCC diagnostic ignored \"-Wnull-dereference\"") *     \
			(int *)0 = 0;                                                      \
		_Pragma("GCC diagnostic pop")                                          \
	}

#define arraylength(array) (sizeof(array) / sizeof((array)[0]))

#define Kilobytes(x) ((x) * (size_t)1024)
#define Megabytes(x) (Kilobytes(x) * (size_t)1024)
#define Gigabytes(x) (Megabytes(x) * (size_t)1024)
#define Terabytes(x) (Gigabytes(x) * (size_t)1024)

#define MAX_CONTROLLERS 4

#define global_variable static
#define local_persist static

//  NOTE(bruno): services that the platform layer provides to the game layer
//  -----------------------------------------------------------------
//  -----------------------------------------------------------------

#if HANDMADE_INTERNAL
struct DEBUGReadFileResult {
	size_t size;
	void *data;
};

typedef DEBUGReadFileResult (*DEBUGPlatformReadEntireFileFunc)(const char *);
typedef void (*DEBUGPlatformFreeFileMemoryFunc)(void *);
typedef bool (*DEBUGPlatformWriteEntireFileFunc)(const char *, uint32, void *);
#endif

//  NOTE(bruno): services that the game layer provides to the platform layer
//  -----------------------------------------------------------------
//  -----------------------------------------------------------------

struct GameMemory {
	size_t permanentStorageSize;
	void *permanentStorage;

	size_t transientStorageSize;
	void *transientStorage;

	bool isInitialized;

	DEBUGPlatformReadEntireFileFunc DEBUGPlatformReadEntireFile;
	DEBUGPlatformFreeFileMemoryFunc DEBUGPlatformFreeFileMemory;
	DEBUGPlatformWriteEntireFileFunc DEBUGPlatformWriteEntireFile;
};

struct GameBackbuffer {
	int width;
	int height;
	int pitch;
	void *memory;
};

struct GameSoundBuffer {
	int16 *samples;
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
	GameButtonState mouseButtons[5];
	int32 mouseX, mouseY, mouseZ;

	real32 deltaTime;
	GameControllerInput controllers[MAX_CONTROLLERS + 1];
};

struct ChunkPosition {
	uint32 chunkX;
	uint32 chunkY;
	uint32 tileX;
	uint32 tileY;
};

struct WorldPosition {
#if 0
	int32 tilemapX;
	int32 tilemapY;
	int32 tileX;
	int32 tileY;
	real32 tileRelX;
	real32 tileRelY;
#else
	uint32 tileX;
	uint32 tileY;
	real32 tileOffsetX;
	real32 tileOffsetY;
#endif
};

struct GameState {
	real32 tsine;
	WorldPosition playerPos;
};

struct Chunk {
	uint32 *tiles;
};

struct Tilemap {
	uint32 *tiles;
};

struct World {
	Chunk *chunks;
	real32 tileSideInMeters;
	uint32 tileSideInPixels;
	real32 metersToPixels;

	int32 lowerLeftX;
	int32 lowerLeftY;

	int32 chunkShift;
	int32 chunkMask;
	uint32 chunkSize;
};

inline GameControllerInput *gameGetController(GameInput *input, size_t index) {
	assert(index >= 0 && index < arraylength(input->controllers));

	GameControllerInput *controller = &input->controllers[index];

	// TODO(bruno): better assert here
	assert(&controller->start - &controller->buttons[0] ==
		   arraylength(controller->buttons) - 1);

	return controller;
}

inline uint32 safeTruncateUint64(uint64 value) {
	assert(value <=
		   0xFFFFFFFF); // TODO(bruno): defines for max values like u_int32_max
	uint32 result = (uint32)value;
	return result;
}

// TODO(bruno): work with stubs so that platform can still boot if no game code
// is found
typedef void (*GAME_UPDATE_AND_RENDER)(GameMemory *, GameBackbuffer *,
									   GameSoundBuffer *, GameInput *);

extern "C" {
void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input);
}

#define HANDMADE_H
#endif // HANDMADE_H
