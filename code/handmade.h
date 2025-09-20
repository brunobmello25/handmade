#if !defined(HANDMADE_H)

// NOTE(Bruno): Services that the game provider to the platform layer

struct GameBackBuffer
{
	void *memory;
	int width;
	int height;
	int pitch;
};

internal void GameUpdateAndRender(GameBackBuffer *backBuffer, int blueOffset,
								  int greenOffset);

// TODO(Bruno): Services that the platform layer provides to the game

#define HANDMADE_H
#endif
