#include "handmade.h"
#include <math.h>

global_variable real32 PI = 3.14159265359f;

internal void renderWeirdGradient(GameBackbuffer *buffer, int blueOffset,
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

internal void gameOutputSound(GameSoundBuffer *soundBuffer, int toneHz) {
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

internal void gameUpdateAndRender(GameBackbuffer *backbuffer,
								  GameSoundBuffer *soundBuffer,
								  GameInput *input) {
	// TODO(bruno): Remove these local persists
	local_persist int blueOffset = 0;
	local_persist int greenOffset = 0;
	local_persist int toneHz = 256;

	GameControllerInput *input0 = &input->controllers[0];
	if (input0->isAnalog) {
		blueOffset += (int)(4.0f * input0->endX);
		greenOffset += (int)(4.0f * input0->endY);
		toneHz = 256 + (int)(256.0f * (input0->endY / 8.0f));
	} else {
	}

	if (input0->down.endedDown) {
		greenOffset += 1;
	}
	if (input0->up.endedDown) {
		greenOffset -= 1;
	}
	if (input0->left.endedDown) {
		blueOffset -= 1;
	}
	if (input0->right.endedDown) {
		blueOffset += 1;
	}

	gameOutputSound(soundBuffer,
					toneHz); // TODO(bruno): Allow sample offsets here
							 // for more robust platform options

	renderWeirdGradient(backbuffer, blueOffset, greenOffset);
}
