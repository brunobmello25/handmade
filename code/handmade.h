#if !defined(HANDMADE_H)

#define Pi32 3.14159265359f

/*
 * NOTE(casey):
 *
 * HANDMADE_INTERNAL:
 *	0 - Build for public release
 *	1 - Build for developer only
 *
 * HANDMADE_SLOW:
 *	0 - No slow code allowed!
 *	1 - Slow code welcome!
 * */

#if HANDMADE_SLOW
#define assert(expression)                                                     \
	if (!(expression))                                                         \
	{                                                                          \
		(*(int *)0 = 0);                                                       \
	}
#else
#define assert(expression)
#endif

// TODO(casey): swap, min, max macros, maybe?
#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)
#define Terabytes(value) (Gigabytes(value) * 1024)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

// NOTE(casey): Services that the game provider to the platform layer

struct GameBackBuffer
{
	void *memory;
	int width;
	int height;
	int pitch;
};

struct GameSoundBuffer
{
	int sampleRate;
	int sampleCount;
	int16 *samples;
};

struct GameButtonState
{
	int halfTransitionCount;
	bool endedDown;
};

struct GameControllerInput
{
	bool isAnalog;

	real32 startX;
	real32 startY;

	real32 minX;
	real32 minY;

	real32 maxX;
	real32 maxY;

	real32 endX;
	real32 endY;

	union
	{
		GameButtonState buttons[6];
		struct
		{
			GameButtonState up;
			GameButtonState down;
			GameButtonState left;
			GameButtonState right;
			GameButtonState leftShoulder;
			GameButtonState rightShoulder;
		};
	};
};

struct GameInput
{
	GameControllerInput controllers[4];
};

struct GameState
{
	int toneHz;
	int greenOffset;
	int blueOffset;
};

struct GameMemory
{
	bool isInitialized;
	uint64 permanentStorageSize;
	void *permanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at
							// startup
	uint64 transientStorageSize;
	void *transientStorage; // NOTE(casey): REQUIRED to be cleared to
							// zero at startup
};

internal void GameOutputSound(GameSoundBuffer *soundBuffer,
							  int sampleCountToOutput);
internal void GameUpdateAndRender(GameMemory *memory, GameInput *gameInput,
								  GameBackBuffer *backBuffer,
								  GameSoundBuffer *soundBuffer);

// NOTE(casey): Services that the platform layer provides to the game

#if HANDMADE_INTERNAL
struct DEBUGReadFileResult
{
	uint32 contentsSize;
	void *contents;
};

internal DEBUGReadFileResult DEBUGPlatformReadEntireFile(char *filename);
internal void DEBUGPlatformFreeFileMemory(void *memory);

internal bool DEBUGPlatformWriteEntireFile(char *filename, void *memory,
										   uint32 size);
#endif

#define HANDMADE_H
#endif
