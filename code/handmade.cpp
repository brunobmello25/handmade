#include "handmade.h"

// Rendering
internal void RenderWeirdGradient(GameBackBuffer *backbuffer, int BlueOffset,
								  int GreenOffset)
{
	uint8_t *Row = (uint8_t *)backbuffer->memory;

	for (int Y = 0; Y < backbuffer->height; Y++)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = 0; X < backbuffer->width; X++)
		{
			// Memory Order: BB GG RR XX
			// 0xXXBBGGRR
			uint8_t Blue = X + BlueOffset;
			uint8_t Green = Y + GreenOffset;

			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += backbuffer->pitch;
	}
}

internal void GameUpdateAndRender(GameBackBuffer *backBuffer, int blueOffset,
								  int greenOffset)
{
	RenderWeirdGradient(backBuffer, blueOffset, greenOffset);
}
