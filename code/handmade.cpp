#include "handmade.h"

internal void GameOutputSound(GameSoundOutputBuffer *soundBuffer, int toneHz)
{
	local_persist real32 tSine;
	int16 toneVolume = 3000;
	int wavePeriod = soundBuffer->samplesPerSecond / toneHz;

	int16 *sampleOut = soundBuffer->samples;
	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount;
		 ++sampleIndex)
	{
		// TODO(casey): Draw this out for people
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (real32)wavePeriod;
	}
}

internal void RenderWeirdGradient(GameOffscreenBuffer *buffer, int blueOffset,
								  int greenOffset)
{
	// TODO(casey): Let's see what the optimizer does

	uint8 *row = (uint8 *)buffer->memory;
	for (int Y = 0; Y < buffer->height; ++Y)
	{
		uint32 *pixel = (uint32 *)row;
		for (int X = 0; X < buffer->width; ++X)
		{
			uint8 blue = (uint8)(X + blueOffset);
			uint8 green = (uint8)(Y + greenOffset);

			*pixel++ = ((green << 8) | blue);
		}

		row += buffer->pitch;
	}
}

internal void GameUpdateAndRender(GameMemory *memory, GameInput *input,
								  GameOffscreenBuffer *buffer,
								  GameSoundOutputBuffer *soundBuffer)
{
	assert((&input->controllers[0].terminator -
			&input->controllers[0].buttons[0]) ==
		   (ArrayCount(input->controllers[0].buttons)));
	assert(sizeof(GameState) <= memory->permanentStorageSize);

	GameState *gameState = (GameState *)memory->permanentStorage;
	if (!memory->isInitialized)
	{
		char *filename = __FILE__;

		DebugReadFileResult file = DEBUGPlatformReadEntireFile(filename);
		if (file.contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", file.contentsSize,
										 file.contents);
			DEBUGPlatformFreeFileMemory(file.contents);
		}

		gameState->toneHz = 256;

		// TODO(casey): This may be more appropriate to do in the platform layer
		memory->isInitialized = true;
	}

	for (size_t controllerIndex = 0;
		 controllerIndex < ArrayCount(input->controllers); ++controllerIndex)
	{
		GameControllerInput *controller = GetController(input, controllerIndex);
		if (controller->isAnalog)
		{
			// NOTE(casey): Use analog movement tuning
			gameState->blueOffset += (int)(4.0f * controller->stickAverageX);
			gameState->toneHz = 256 + (int)(128.0f * controller->stickAverageY);
		}
		else
		{
			// NOTE(casey): Use digital movement tuning
			if (controller->moveLeft.endedDown)
			{
				gameState->blueOffset -= 1;
			}

			if (controller->moveRight.endedDown)
			{
				gameState->blueOffset += 1;
			}
		}

		// input.AButtonEndedDown;
		// input.AButtonHalfTransitionCount;
		if (controller->actionDown.endedDown)
		{
			gameState->greenOffset += 1;
		}
	}

	// TODO(casey): Allow sample offsets here for more robust platform options
	GameOutputSound(soundBuffer, gameState->toneHz);
	RenderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}
