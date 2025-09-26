#if !defined(HANDMADE_H)

// NOTE(Bruno): Services that the game provider to the platform layer

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

internal void GameOutputSound(GameSoundBuffer *soundBuffer,
							  int sampleCountToOutput);
internal void GameUpdateAndRender(GameBackBuffer *backBuffer, int blueOffset,
								  int greenOffset, GameSoundBuffer *soundBuffer,
								  int toneHz);

// TODO(Bruno): Services that the platform layer provides to the game

#define HANDMADE_H
#endif
