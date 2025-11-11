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

void renderPlayer(GameBackbuffer *buffer, int playerX, int playerY) {
	for (int y = -5; y <= 5; ++y) {
		for (int x = -5; x <= 5; ++x) {
			int pixelX = playerX + x;
			int pixelY = playerY + y;

			// Bounds checking
			if (pixelX >= 0 && pixelX < buffer->width && pixelY >= 0 &&
				pixelY < buffer->height) {
				uint8_t *row =
					(uint8_t *)buffer->memory + (pixelY * buffer->pitch);
				uint32_t *pixel = (uint32_t *)row + pixelX;
				*pixel = 0xFFFFFFFF; // White color (ARGB)
			}
		}
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

		// TODO(bruno): ditch this compile-time flag once we stop debugging
		// audio with a sine wave
#if ENABLE_SINE_WAVE
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
#else
		int16_t sampleValue = 0;
#endif
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
		gameState->playerX = 150;
		gameState->playerY = 150;

		gameMemory->isInitialized = true;
	}

	int gradientSpeed = 4;
	int playerSpeed = 8;

	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
			gameState->xOffset +=
				(int)((real32)gradientSpeed * controller->stickAverageX);
			gameState->yOffset +=
				(int)((real32)gradientSpeed * controller->stickAverageY);
			gameState->toneHz =
				256 + (int)(256.0f * (controller->stickAverageY / 8.0f));

			gameState->playerX +=
				(int)(playerSpeed * controller->stickAverageX);
			gameState->playerY +=
				(int)(playerSpeed * controller->stickAverageY);

			if (gameState->playerX < 0)
				gameState->playerX = backbuffer->width + gameState->playerX;
			if (gameState->playerY < 0)
				gameState->playerY = backbuffer->height + gameState->playerY;
			gameState->playerX %= backbuffer->width;
			gameState->playerY %= backbuffer->height;
		} else {
			if (controller->moveUp.endedDown) {
				gameState->yOffset -= gradientSpeed;
				gameState->playerY -= playerSpeed;
			}
			if (controller->moveDown.endedDown) {
				gameState->yOffset += gradientSpeed;
				gameState->playerY += playerSpeed;
			}
			if (controller->moveLeft.endedDown) {
				gameState->xOffset -= gradientSpeed;
				gameState->playerX -= playerSpeed;
			}
			if (controller->moveRight.endedDown) {
				gameState->xOffset += gradientSpeed;
				gameState->playerX += playerSpeed;
			}
		}

		if (controller->actionDown.endedDown) {
			gameState->playerY -= playerSpeed;
		}
	}

	gameOutputSound(soundBuffer,
					gameState); // TODO(bruno): Allow sample offsets
								// here for more robust platform options

	renderWeirdGradient(backbuffer, gameState->xOffset, gameState->yOffset);
	renderPlayer(backbuffer, gameState->playerX, gameState->playerY);

	// Render cursor at mouse position
	renderPlayer(backbuffer, input->mouseX, input->mouseY);

	// Render squares for each mouse button that's pressed
	for (size_t i = 0; i < arraylength(input->mouseButtons); ++i) {
		if (input->mouseButtons[i].endedDown) {
			renderPlayer(backbuffer, 10 + 20 * i, 10);
		}
	}
}
