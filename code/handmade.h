#if !defined(HANDMADE_H)

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

internal void GameOutputSound(GameSoundBuffer *soundBuffer,
							  int sampleCountToOutput);
internal void GameUpdateAndRender(GameInput *gameInput,
								  GameBackBuffer *backBuffer,
								  GameSoundBuffer *soundBuffer);

// TODO(casey): Services that the platform layer provides to the game

#define HANDMADE_H
#endif
