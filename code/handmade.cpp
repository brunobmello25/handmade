#include "handmade.h"

void renderWeirdGradient(GameBackbuffer *buffer, int blueOffset,
						 int greenOffset) {
	uint8_t *row = (uint8_t *)buffer->memory;
	for (int y = 0; y < buffer->height; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		for (int x = 0; x < buffer->width; ++x) {
			uint8_t blue = (x + blueOffset);
			uint8_t green = (y + greenOffset);

			*pixel++ = (0xFF << 24) | (green << 8) | blue;
		}

		row += buffer->pitch;
	}
}

void gameUpdateAndRender(GameBackbuffer *buffer, int xOffset, int yOffset) {
	renderWeirdGradient(buffer, xOffset, yOffset);
}
