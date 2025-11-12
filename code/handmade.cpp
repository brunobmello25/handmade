#include "handmade.h"
#include <math.h>

global_variable real32 PI = 3.14159265359f;

int32 roundReal32ToInt32(real32 value) { return (int32)(value + 0.5f); }
uint32 roundReal32ToUInt32(real32 value) { return (uint32)(value + 0.5f); }

void renderRectangle(GameBackbuffer *buffer, real32 minXf, real32 minYf,
					 real32 maxXf, real32 maxYf, real32 R, real32 G, real32 B) {
	int32 minX = roundReal32ToInt32(minXf);
	int32 minY = roundReal32ToInt32(minYf);
	int32 maxX = roundReal32ToInt32(maxXf);
	int32 maxY = roundReal32ToInt32(maxYf);
	if (minX < 0) minX = 0;
	if (minY < 0) minY = 0;
	if (maxX > buffer->width) maxX = buffer->width;
	if (maxY > buffer->height) maxY = buffer->height;

	uint32 color =
		(((uint32)255.0f << 24) | (roundReal32ToUInt32(R * 255.0f) << 16) |
		 (roundReal32ToUInt32(G * 255.0f) << 8) |
		 (roundReal32ToUInt32(B * 255.0f) << 0));

	size_t bytesPerPixel = sizeof(uint32);

	uint8 *row =
		(uint8 *)buffer->memory + minX * bytesPerPixel + minY * buffer->pitch;

	for (int y = minY; y < maxY; y++) {
		uint32 *pixel = (uint32 *)row;
		for (int x = minX; x < maxX; x++) {
			*pixel++ = color;
		}
		row += buffer->pitch;
	}
}

void gameOutputSound(GameSoundBuffer *soundBuffer, GameState *gameState) {
	if (soundBuffer->sampleCount == 0) return;

	int toneHz = 256;
	int wavePeriod = soundBuffer->sampleRate / toneHz;

	int16 *sampleOut = soundBuffer->samples;

	for (int i = 0; i < soundBuffer->sampleCount; i++) {
		// TODO(bruno): ditch this compile-time flag once we stop debugging
		// audio with a sine wave
#if ENABLE_SINE_WAVE
		int toneVolume = 3000;
		real32 sineValue = sinf(gameState->tsine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
#else
		int16 sampleValue = 0;
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
		gameState->tsine = 0.0f;
		gameState->playerX = 50.0f;
		gameState->playerY = 50.0f;

		gameMemory->isInitialized = true;
	}

	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
		} else {
			real32 speed = 100.0f * input->deltaTime;
			if (controller->moveDown.endedDown) gameState->playerY += speed;
			if (controller->moveUp.endedDown) gameState->playerY -= speed;
			if (controller->moveLeft.endedDown) gameState->playerX -= speed;
			if (controller->moveRight.endedDown) gameState->playerX += speed;
		}
	}

	uint32 tilemap[9][16] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	};

	gameOutputSound(soundBuffer,
					gameState); // TODO(bruno): Allow sample offsets
								// here for more robust platform options

	renderRectangle(backbuffer, 0, 0, backbuffer->width, backbuffer->height, 1,
					0, 1);

	real32 upperLeftX = 0.0f;
	real32 upperLeftY = 0.0f;
	real32 tileWidth = 60.0f;
	real32 tileHeight = 60.0f;
	for (int row = 0; row < 9; row++) {
		for (int col = 0; col < 16; col++) {
			uint32 tileID = tilemap[row][col];
			real32 gray = 0.5f;
			if (tileID == 1) {
				gray = 1.0f;
			}
			real32 minX = upperLeftX + col * tileWidth;
			real32 minY = upperLeftY + row * tileHeight;
			real32 maxX = minX + tileWidth;
			real32 maxY = minY + tileHeight;
			renderRectangle(backbuffer, minX, minY, maxX, maxY, gray, gray,
							gray);
		}
	}

	real32 playerR = 0.0f;
	real32 playerG = 1.0f;
	real32 playerB = 1.0f;
	real32 playerWidth = 0.75f * tileWidth;
	real32 playerHeight = 0.75f * tileHeight;
	real32 playerLeft = gameState->playerX - (playerWidth / 2);
	real32 playerTop = gameState->playerY - (playerHeight);
	real32 playerRight = playerLeft + playerWidth;
	real32 playerBottom = playerTop + playerHeight;
	renderRectangle(backbuffer, playerLeft, playerTop, playerRight,
					playerBottom, playerR, playerG, playerB);
}
