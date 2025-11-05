#ifndef HANDMADE_H

#include <stdint.h>

#define MAX_CONTROLLERS 4

#define global_variable static
#define local_persist static

typedef float real32;
typedef double real64;

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

// receives: timing, input, bitmap to output, sound to output
void gameUpdateAndRender(GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input);

void gameOutputSound(GameSoundBuffer *soundBuffer, int toneHz);

#define HANDMADE_H
#endif // HANDMADE_H
