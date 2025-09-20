#include "handmade.h"

// Rendering
internal void RenderWeirdGradient(GameBackBuffer *backBuffer, int blueOffset,
								  int greenOffset)
{
	uint8_t *row = (uint8_t *)backBuffer->memory;
	for (int y = 0; y < backBuffer->height; y++)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int x = 0; x < backBuffer->width; x++)
		{
			uint8_t blue = (x + blueOffset);
			uint8_t green = (y + greenOffset);

			*pixel++ = (0xFF << 24) | (blue << 16) | (green << 8) | 0;
		}
		row += backBuffer->pitch;
	}
}

internal void GameUpdateAndRender(GameBackBuffer *backBuffer, int blueOffset,
								  int greenOffset)
{
	RenderWeirdGradient(backBuffer, blueOffset, greenOffset);
}
