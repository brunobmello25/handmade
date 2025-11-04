#ifndef HANDMADE_H

#include <stdint.h>

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

// receives: timing, input, bitmap to output, sound to output
void gameUpdateAndRender(GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer);

void gameOutputSound(GameSoundBuffer *soundBuffer);

#define HANDMADE_H
#endif // HANDMADE_H
