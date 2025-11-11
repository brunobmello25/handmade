#include "handmade.h"
#include <math.h>

global_variable real32 PI = 3.14159265359f;

int32 roundReal32ToInt32(real32 value) { return (int32)(value + 0.5f); }

void renderRectangle(GameBackbuffer *buffer, real32 minXf, real32 minYf,
					 real32 maxXf, real32 maxYf, uint32 color) {
	int32 minX = roundReal32ToInt32(minXf);
	int32 minY = roundReal32ToInt32(minYf);
	int32 maxX = roundReal32ToInt32(maxXf);
	int32 maxY = roundReal32ToInt32(maxYf);
	if (minX < 0) minX = 0;
	if (minY < 0) minY = 0;
	if (maxX > buffer->width) maxX = buffer->width;
	if (maxY > buffer->height) maxY = buffer->height;

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

	int wavePeriod = soundBuffer->sampleRate / gameState->toneHz;

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
		gameState->toneHz = 256;
		gameState->tsine = 0.0f;

		gameMemory->isInitialized = true;
	}

	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
		} else {
		}
	}

	gameOutputSound(soundBuffer,
					gameState); // TODO(bruno): Allow sample offsets
								// here for more robust platform options

	renderRectangle(backbuffer, 0, 0, backbuffer->width, backbuffer->height,
					0xFFFF00FF);
	renderRectangle(backbuffer, 30, 30, 100, 100, 0xFF00FFFF);
}
