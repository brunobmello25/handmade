#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define global_variable static
#define MAX_CONTROLLERS 4
#define STICK_DEADZONE 8000

typedef float real32;
typedef double real64;

struct PlatformBackbuffer {
	int width;
	int height;
	int pitch;
	void *memory;
	SDL_Texture *texture;
};

struct PlatformAudioSettings {
	int sampleRate;
	int bytesPerSample;
	int numChannels;

	int toneHz;
	int toneVolume;
	int wavePeriod;
	int sampleIndex;
	int latencySampleCount;
	float tSine;
};

struct PlatformAudioBuffer {
	int8_t *buffer; // TODO(bruno): switch this to a void* maybe
	int size;
	int readCursor;
	int writeCursor;
	PlatformAudioSettings settings;
	SDL_AudioStream *stream;
};

global_variable bool globalRunning;

global_variable PlatformBackbuffer globalBackbuffer;

global_variable PlatformAudioBuffer globalAudioBuffer;

global_variable SDL_Gamepad *GamepadHandles[MAX_CONTROLLERS] = {};
global_variable int xOffset = 0, yOffset = 0;

global_variable float PI = 3.14159265359f;
global_variable float TAU = 2 * PI;

// function that gets called everytime the window size changes, and also
// at first time, in order to allocate the backbuffer with proper dimensions
void platformResizeBackbuffer(PlatformBackbuffer *backbuffer,
							  SDL_Renderer *renderer, int width, int height) {
	int bytesPerPixel = sizeof(int);

	if (backbuffer->memory)
		free(backbuffer->memory);

	if (backbuffer->texture)
		SDL_DestroyTexture(backbuffer->texture);

	backbuffer->width = width;
	backbuffer->height = height;
	backbuffer->pitch = width * bytesPerPixel;
	backbuffer->memory = malloc(width * height * bytesPerPixel);
	backbuffer->texture =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, width, height);
}

bool platformProcessEvents(PlatformBackbuffer *backbuffer) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			return false;
		}
		if (event.type == SDL_EVENT_KEY_UP ||
			event.type == SDL_EVENT_KEY_DOWN) {
			// TODO(bruno): handle key wasdown and isdown
			if (event.key.key == SDLK_ESCAPE)
				return false;

			if (event.key.key == SDLK_W)
				printf("w\n");
			if (event.key.key == SDLK_A)
				printf("a\n");
			if (event.key.key == SDLK_S)
				printf("s\n");
			if (event.key.key == SDLK_D)
				printf("d\n");
		}
		if (event.type == SDL_EVENT_WINDOW_RESIZED) {
			SDL_Window *window = SDL_GetWindowFromEvent(&event);

			if (!window)
				return false; // TODO(bruno): proper error handling

			SDL_Renderer *renderer = SDL_GetRenderer(window);
			if (!renderer)
				return false; // TODO(bruno): proper error handling

			platformResizeBackbuffer(backbuffer, renderer, event.window.data1,
									 event.window.data2);
		}
	}
	return true;
}

void platformUpdateWindow(PlatformBackbuffer *buffer, SDL_Window *window,
						  SDL_Renderer *renderer) {
	SDL_UpdateTexture(buffer->texture, NULL, buffer->memory, buffer->pitch);
	SDL_RenderTexture(renderer, buffer->texture, 0, 0);
	SDL_RenderPresent(renderer);
}

void renderWeirdGradient(PlatformBackbuffer *buffer, int blueOffset,
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

void platformLoadControllers() {

	int gamepadCount;
	uint *ids = SDL_GetGamepads(&gamepadCount);

	for (int i = 0; i < gamepadCount && i < MAX_CONTROLLERS; i++) {
		SDL_Gamepad *pad = SDL_OpenGamepad(ids[i]);
		if (pad) {
			GamepadHandles[i] = pad;
		}
	}
}

void platformAudioCallback(void *userdata, SDL_AudioStream *stream, int amount,
						   int totalAmount) {
	PlatformAudioBuffer *audioBuffer = (PlatformAudioBuffer *)userdata;

	int region1size = amount;
	int region2size = 0;
	if (audioBuffer->readCursor + region1size > audioBuffer->size) {
		region1size = audioBuffer->size - audioBuffer->readCursor;
		region2size = amount - region1size;
	}

	SDL_PutAudioStreamData(
		stream, (void *)&audioBuffer->buffer[audioBuffer->readCursor],
		region1size);
	if (region2size > 0) {
		SDL_PutAudioStreamData(stream, audioBuffer->buffer, region2size);
	}

	audioBuffer->readCursor =
		(audioBuffer->readCursor + amount) % audioBuffer->size;
}

void platformInitializeSound(PlatformAudioBuffer *audioBuffer) {

	audioBuffer->settings = {};
	audioBuffer->settings.sampleRate = 48000;
	audioBuffer->settings.numChannels = 2;
	audioBuffer->settings.sampleIndex = 0;
	audioBuffer->settings.toneHz = 256;
	audioBuffer->settings.toneVolume = 3000;
	audioBuffer->settings.tSine = 0.0f;
	audioBuffer->settings.latencySampleCount =
		audioBuffer->settings.sampleRate / 15;
	audioBuffer->settings.bytesPerSample =
		audioBuffer->settings.numChannels * sizeof(int16_t);
	audioBuffer->settings.wavePeriod =
		audioBuffer->settings.sampleRate / audioBuffer->settings.toneHz;

	int secondsOfAudio = 1;
	audioBuffer->size = secondsOfAudio * audioBuffer->settings.sampleRate *
						audioBuffer->settings.bytesPerSample;

	audioBuffer->buffer = (int8_t *)calloc(sizeof(int8_t), audioBuffer->size);

	if (!audioBuffer->buffer) {
		printf("error allocating audio buffer\n");
		return;
	}

	// start write cursor 50ms worth of samples ahead of read cursor:
	// 1000ms (1seg) / 20 = 50ms
	// samplerate(48000) / 20 = 2400 samples
	// 2400 samples * 4 bytes per sample (stereo 16bit) = 9600 bytes
	audioBuffer->readCursor = 0;
	audioBuffer->writeCursor = 0;
	// audioBuffer->writeCursor = audioBuffer->settings.sampleRate / 20 *
	// 						   audioBuffer->settings.bytesPerSample;

	SDL_AudioSpec specs = {};
	specs.format = SDL_AUDIO_S16LE;
	specs.channels = 2;
	specs.freq = audioBuffer->settings.sampleRate;

	audioBuffer->stream =
		SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specs,
								  &platformAudioCallback, audioBuffer);
	if (!audioBuffer->stream) {
		const char *error = SDL_GetError();
		printf("error opening sdl audio stream: %s\n", error);
		return;
	}

	SDL_ResumeAudioStreamDevice(audioBuffer->stream);
}

int16_t platformGetAxisWithDeadzone(SDL_Gamepad *pad, SDL_GamepadAxis axis) {
	int16_t axisValue = SDL_GetGamepadAxis(pad, axis);
	if (abs(axisValue) < STICK_DEADZONE) {
		axisValue = 0;
	}
	return axisValue;
}

void platformProcessControllers() {
	for (int i = 0; i < MAX_CONTROLLERS; i++) {
		SDL_Gamepad *pad = GamepadHandles[i];
		if (!pad)
			continue;

		int16_t stickX =
			platformGetAxisWithDeadzone(pad, SDL_GAMEPAD_AXIS_LEFTX);
		int16_t stickY =
			platformGetAxisWithDeadzone(pad, SDL_GAMEPAD_AXIS_LEFTY);

		xOffset += stickX / 8192;
		yOffset += stickY / 8192;

		globalAudioBuffer.settings.toneHz =
			256.0f + (int)(128.0f * ((float)stickY / 30000.0f));
		globalAudioBuffer.settings.wavePeriod =
			globalAudioBuffer.settings.sampleRate /
			globalAudioBuffer.settings.toneHz;

		bool aButton = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
		if (aButton) {
			yOffset += 2;
			SDL_RumbleGamepad(pad, 20000, 20000, 30);
		} else {
			SDL_RumbleGamepad(pad, 0, 0, 0);
		}
	}
}

void platformFillSoundBuffer(PlatformAudioBuffer *audioBuffer, int byteToLock,
							 int bytesToWrite) {
	void *region1 = (u_int8_t *)audioBuffer->buffer + byteToLock;
	int region1size = bytesToWrite;
	if (region1size + byteToLock > audioBuffer->size)
		region1size = audioBuffer->size - byteToLock;

	void *region2 = audioBuffer->buffer;
	int region2size = bytesToWrite - region1size;

	int region1SampleCount = region1size / audioBuffer->settings.bytesPerSample;
	int16_t *sampleOut = (int16_t *)region1;

	for (int i = 0; i < region1SampleCount; i++) {
		real32 sineValue = sinf(audioBuffer->settings.tSine);
		int16_t sampleValue =
			(int16_t)(sineValue * audioBuffer->settings.toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		audioBuffer->settings.tSine +=
			2.0f * PI * 1.0f / (real32)audioBuffer->settings.wavePeriod;

		audioBuffer->settings.sampleIndex++;
	}

	int region2SampleCount = region2size / audioBuffer->settings.bytesPerSample;
	sampleOut = (int16_t *)region2;
	for (int i = 0; i < region2SampleCount; ++i) {
		real32 sineValue = sinf(audioBuffer->settings.tSine);
		int16_t sampleValue =
			(int16_t)(sineValue * audioBuffer->settings.toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		audioBuffer->settings.tSine +=
			2.0f * PI * 1.0f / (real32)audioBuffer->settings.wavePeriod;
		audioBuffer->settings.sampleIndex++;
	}
}

int main(void) {
	int initialWidth = 1920 / 2;
	int initialHeight = 1080 / 2;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD |
				  SDL_INIT_AUDIO))
		return -1;

	platformLoadControllers();

	SDL_Window *window = SDL_CreateWindow("Handmade Hero", initialWidth,
										  initialHeight, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
	if (!window || !renderer) // TODO(bruno): proper error handling
		return -1;

	platformInitializeSound(&globalAudioBuffer);
	platformFillSoundBuffer(&globalAudioBuffer, 0,
							globalAudioBuffer.settings.latencySampleCount *
								globalAudioBuffer.settings.bytesPerSample);

	globalRunning = true;

	globalBackbuffer = {};
	platformResizeBackbuffer(&globalBackbuffer, renderer, initialWidth,
							 initialHeight);

	while (globalRunning) {
		globalRunning = platformProcessEvents(&globalBackbuffer);

		SDL_LockAudioStream(globalAudioBuffer.stream);
		int byteToLock = (globalAudioBuffer.settings.sampleIndex *
						  globalAudioBuffer.settings.bytesPerSample) %
						 globalAudioBuffer.size;
		int targetCursor = ((globalAudioBuffer.readCursor +
							 (globalAudioBuffer.settings.latencySampleCount *
							  globalAudioBuffer.settings.bytesPerSample)) %
							globalAudioBuffer.size);
		int bytesToWrite;
		if (byteToLock > targetCursor) {
			bytesToWrite = (globalAudioBuffer.size - byteToLock);
			bytesToWrite += targetCursor;
		} else {
			bytesToWrite = targetCursor - byteToLock;
		}
		platformFillSoundBuffer(&globalAudioBuffer, byteToLock, bytesToWrite);
		SDL_UnlockAudioStream(globalAudioBuffer.stream);

		platformProcessControllers();

		renderWeirdGradient(&globalBackbuffer, xOffset, yOffset);
		platformUpdateWindow(&globalBackbuffer, window, renderer);
	}

	// TODO(bruno): we are not freeing sdl renderer, sdl window and backbuffer
	// here because honestly the OS will handle this for us after this return 0.
	// but maybe we should revisit this?
	// --
	// nah, probably not

	return 0;
}
