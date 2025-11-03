#include "handmade.h"
#include <math.h>

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

void gameOutputSound(GameSoundBuffer *soundBuffer) {
	local_persist real32 tsine =
		0.0f; // TODO(bruno): remove this local persist variable

	int toneVolume = 3000;
	int tonehz = 256;
	int wavePeriod = soundBuffer->sampleRate / tonehz;

	int16_t *sampleOut = soundBuffer->samples;

	for (int i = 0; i < soundBuffer->sampleCount; i++) {
		real32 sineValue = sinf(tsine);
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tsine += 2.0f * PI * 1.0f / (real32)wavePeriod;
	}
}

void gameUpdateAndRender(GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, int xOffset,
						 int yOffset) {

	// TODO(bruno): Allow sample offsets here for more robust platform options
	gameOutputSound(soundBuffer);

	renderWeirdGradient(backbuffer, xOffset, yOffset);
}
