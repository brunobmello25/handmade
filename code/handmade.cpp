#ifndef HANDMADE_CPP

#include "handmade.h"
#include <math.h>

// FIXME:(bruno): extrair os defines que estão em sdl_handmade.cpp para
// um header genérico. Isso porque eles também são usados aqui, e ter
// esses caras definidos na platform layer da trabalho pra porra pra consertar
// o LSP. Melhor só extrair pra um header genérico mesmo.
// --
// Atenção para não definir nada que seja platform specific

internal void GameOutputSound(GameSoundBuffer *soundBuffer, int toneHz)
{
	local_persist float tSine;
	int16 toneVolume = 3000;
	int wavePeriod = soundBuffer->sampleRate / toneHz;

	int16 *samples = soundBuffer->samples;

	for (int SampleIndex = 0; SampleIndex < soundBuffer->sampleCount;
		 ++SampleIndex)
	{
		float sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*samples++ = sampleValue;
		*samples++ = sampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (float)wavePeriod;
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

internal void GameUpdateAndRender(GameMemory *memory, GameInput *gameInput,
								  GameBackBuffer *backBuffer,
								  GameSoundBuffer *soundBuffer)
{
	assert(sizeof(GameState) <= memory->permanentStorageSize);

	GameState *gameState = (GameState *)memory->permanentStorage;
	if (!memory->isInitialized)
	{
		const char *fileName = "test.bmp";

		DEBUGReadFileResult fileResult = DEBUGPlatformReadEntireFile(fileName);
		if (fileResult.contents)
		{
			DEBUGPlatformFreeFileMemory(fileResult.contents);
		}

		gameState->toneHz = 256;

		// TOOD(casey): might be more appropriate to do this bool initialized
		// variable in the platform layer
		memory->isInitialized = true;
	}

	GameControllerInput input0 = gameInput->controllers[0];
	if (input0.isAnalog)
	{
		gameState->blueOffset += (int)4.0f * (input0.endX);
		gameState->toneHz = 256 + (int)(128.0f * (input0.endY));
	}
	else
	{
	}

	if (input0.down.endedDown)
	{
		gameState->greenOffset += 1;
	}

	// TODO(casey): allow sampleoffsets here
	GameOutputSound(soundBuffer, gameState->toneHz);
	RenderWeirdGradient(backBuffer, gameState->blueOffset,
						gameState->greenOffset);
}
#define HANDMADE_CPP
#endif
