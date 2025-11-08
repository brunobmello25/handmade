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

void gameOutputSound(GameSoundBuffer *soundBuffer, GameState *gameState) {
	if (soundBuffer->sampleCount == 0)
		return;

	int toneVolume = 3000;
	int wavePeriod = soundBuffer->sampleRate / gameState->toneHz;

	int16_t *sampleOut = soundBuffer->samples;

	for (int i = 0; i < soundBuffer->sampleCount; i++) {
		real32 sineValue = sinf(gameState->tsine);
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		gameState->tsine += 2.0f * PI * 1.0f / (real32)wavePeriod;
		if (gameState->tsine > 2.0f * PI) {
			gameState->tsine -= 2.0f * PI;
		}
	}
}

void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input) {
	assert(sizeof(GameState) <= gameMemory->permanentStorageSize);

	GameState *gameState = (GameState *)gameMemory->permanentStorage;
	if (!gameMemory->isInitialized) {
		const char *filename = "README.md";
		DEBUGReadFileResult file =
			gameMemory->DEBUGPlatformReadEntireFile(filename);
		if (file.size > 0) {
			const char *filename2 = "teste.md";
			gameMemory->DEBUGPlatformWriteEntireFile(filename2, file.size,
													 file.data);
			gameMemory->DEBUGPlatformFreeFileMemory(file.data);
		}

		gameState->toneHz = 256;
		gameState->xOffset = 0;
		gameState->yOffset = 0;
		gameState->tsine = 0.0f;

		gameMemory->isInitialized = true;
	}

	int moveSpeed = 8;
	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
			gameState->xOffset +=
				(int)((real32)moveSpeed * controller->stickAverageX);
			gameState->yOffset +=
				(int)((real32)moveSpeed * controller->stickAverageY);
			gameState->toneHz =
				256 + (int)(256.0f * (controller->stickAverageY / 8.0f));
		} else {
			if (controller->moveUp.endedDown)
				gameState->yOffset -= moveSpeed;
			if (controller->moveDown.endedDown)
				gameState->yOffset += moveSpeed;
			if (controller->moveLeft.endedDown)
				gameState->xOffset -= moveSpeed;
			if (controller->moveRight.endedDown)
				gameState->xOffset += moveSpeed;
		}
	}

	gameOutputSound(soundBuffer,
					gameState); // TODO(bruno): Allow sample offsets
								// here for more robust platform options

	renderWeirdGradient(backbuffer, gameState->xOffset, gameState->yOffset);
}
