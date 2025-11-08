#include "handmade.h"
#include <math.h>

global_variable real32 PI = 3.14159265359f;

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

void gameOutputSound(GameSoundBuffer *soundBuffer, int toneHz) {
	if (soundBuffer->sampleCount == 0)
		return;

	local_persist real32 tsine =
		0.0f; // TODO(bruno): remove this local persist variable

	int toneVolume = 3000;
	int wavePeriod = soundBuffer->sampleRate / toneHz;

	int16_t *sampleOut = soundBuffer->samples;

	for (int i = 0; i < soundBuffer->sampleCount; i++) {
		real32 sineValue = sinf(tsine);
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tsine += 2.0f * PI * 1.0f / (real32)wavePeriod;
	}
}

void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input) {
	assert(sizeof(GameState) <= gameMemory->permanentStorageSize);

	GameState *gameState = (GameState *)gameMemory->permanentStorage;
	if (!gameMemory->isInitialized) {
		const char *filename = "README.md";
		DEBUGReadFileResult file = DEBUGPlatformReadEntireFile(filename);
		if (file.size > 0) {
			const char *filename2 = "teste.md";
			DEBUGPlatformWriteEntireFile(filename2, file.size, file.data);
			DEBUGPlatformFreeFileMemory(file.data);
		}

		gameState->toneHz = 256;
		gameState->xOffset = 0;
		gameState->yOffset = 0;

		gameMemory->isInitialized = true;
	}

	GameControllerInput *input0 =
		&input->controllers[1]; // TODO(bruno): remove this hardcoded index
	if (input0->isAnalog) {
		gameState->xOffset += (int)(4.0f * input0->stickAverageX);
		gameState->yOffset += (int)(4.0f * input0->stickAverageY);
		gameState->toneHz =
			256 + (int)(256.0f * (input0->stickAverageY / 8.0f));
	} else {
		if (input0->moveUp.endedDown)
			gameState->yOffset -= 4;
		if (input0->moveDown.endedDown)
			gameState->yOffset += 4;
		if (input0->moveLeft.endedDown)
			gameState->xOffset -= 4;
		if (input0->moveRight.endedDown)
			gameState->xOffset += 4;
	}

	gameOutputSound(soundBuffer,
					gameState->toneHz); // TODO(bruno): Allow sample offsets
										// here for more robust platform options

	renderWeirdGradient(backbuffer, gameState->xOffset, gameState->yOffset);
}
