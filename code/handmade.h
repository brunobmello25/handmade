#ifndef HANDMADE_H
#define HANDMADE_H

#include <stdint.h>

struct GameBackbuffer {
	int width;
	int height;
	int pitch;
	void *memory;
};

// receives: timing, input, bitmap to output, sound to output
void gameUpdateAndRender(GameBackbuffer *buffer, int xOffset, int yOffset);

#endif // HANDMADE_H
