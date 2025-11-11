#ifndef SDL3_HANDMADE_H

#include "handmade.h"
#include <SDL3/SDL.h>

struct PlatformBackbuffer {
	int width;
	int height;
	int pitch;
	void *memory;
	SDL_Texture *texture;
};

struct PlatformAudioOutput {
	SDL_AudioDeviceID device;
	SDL_AudioStream *stream;
	int sampleRate;
	int numChannels;
};

struct PlatformGameCode {
	void *gameLib;
	GAME_UPDATE_AND_RENDER gameUpdateAndRender;
	time_t lastModTime;
	bool loaded;
};

struct PlatformState {
	int inputRecordingIndex;
	int inputPlayingIndex;

	int inputPlaybackHandle;
	int inputRecordingHandle;

	void *gamePermanentStorage;
	size_t permanentStorageSize;
};

#define SDL3_HANDMADE_H
#endif // SDL3_HANDMADE_H
