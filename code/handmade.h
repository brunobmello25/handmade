#if !defined(HANDMADE_H)
/*
  NOTE(casey):

  HANDMADE_INTERNAL:
		0 - Build for public release
		1 - Build for developer only

  HANDMADE_SLOW:
		0 - Not slow code allowed!
		1 - Slow code welcome.
*/

#if HANDMADE_SLOW
// TODO(casey): Complete assertion macro - don't worry everyone!
#define assert(Expression)                                                     \
	if (!(Expression))                                                         \
	{                                                                          \
		*(int *)0 = 0;                                                         \
	}
#else
#define assert(Expression)
#endif

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(casey): swap, min, max ... macros???

inline uint32 SafeTruncateUInt64(uint64 value)
{
	// TODO(casey): Defines for maximum values
	assert(value <= 0xFFFFFFFF);
	uint32 result = (uint32)value;
	return (result);
}

/*
  NOTE(casey): Services that the platform layer provides to the game
*/
#if HANDMADE_INTERNAL
/* IMPORTANT(casey):

   These are NOT for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data!
*/
struct DebugReadFileResult
{
	uint32 contentsSize;
	void *contents;
};
internal DebugReadFileResult DEBUGPlatformReadEntireFile(char *filename);
internal void DEBUGPlatformFreeFileMemory(void *memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize,
											 void *memory);
#endif

/*
  NOTE(casey): Services that the game provides to the platform layer.
  (this may expand in the future - sound on separate thread, etc.)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound
// buffer to use

// TODO(casey): In the future, rendering _specifically_ will become a
// three-tiered abstraction!!!
struct GameOffscreenBuffer
{
	// NOTE(casey): Pixels are alwasy 32-bits wide, memory Order BB GG RR XX
	void *memory;
	int width;
	int height;
	int pitch;
};

struct GameSoundOutputBuffer
{
	int samplesPerSecond;
	int sampleCount;
	int16 *samples;
};

struct GameButtonState
{
	int halfTransitionCount;
	bool32 endedDown;
};

struct GameControllerInput
{
	bool32 isConnected;
	bool32 isAnalog;
	real32 stickAverageX;
	real32 stickAverageY;

	union
	{
		GameButtonState buttons[12];
		struct
		{
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

			// NOTE(casey): All buttons must be added above this line

			GameButtonState terminator;
		};
	};
};

struct GameInput
{
	// TODO(casey): Insert clock values here.
	GameControllerInput controllers[5];
};
inline GameControllerInput *GetController(GameInput *input,
										  int unsigned controllerIndex)
{
	assert(controllerIndex < ArrayCount(input->controllers));

	GameControllerInput *result = &input->controllers[controllerIndex];
	return (result);
}

struct GameMemory
{
	bool32 isInitialized;

	uint64 permanentStorageSize;
	void *permanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at
							// startup

	uint64 transientStorageSize;
	void *transientStorage; // NOTE(casey): REQUIRED to be cleared to zero at
							// startup
};

internal void GameUpdateAndRender(GameMemory *memory, GameInput *input,
								  GameOffscreenBuffer *buffer,
								  GameSoundOutputBuffer *soundBuffer);

//
//
//

struct GameState
{
	int toneHz;
	int greenOffset;
	int blueOffset;
};

#define HANDMADE_H
#endif
