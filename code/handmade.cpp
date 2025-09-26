#include "handmade.h"

internal void GameOutputSound(GameSoundBuffer *soundBuffer, int toneHz)
{
	local_persist real32 tSine;
	int16 toneVolume = 3000;
	int wavePeriod = soundBuffer->sampleRate / toneHz;

	int16 *samples = soundBuffer->samples;

	for (int SampleIndex = 0; SampleIndex < soundBuffer->sampleCount;
		 ++SampleIndex)
	{
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*samples++ = sampleValue;
		*samples++ = sampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (real32)wavePeriod;
	}
}

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
								  int greenOffset, GameSoundBuffer *soundBuffer,
								  int toneHz)
{
	// TODO(bruno): allow sampleoffsets here
	GameOutputSound(soundBuffer, toneHz);
	RenderWeirdGradient(backBuffer, blueOffset, greenOffset);
}
