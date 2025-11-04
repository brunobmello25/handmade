/*
 * TODO(bruno): Finish this platform layer
 * - save game locations
 * - getting a handle to our own executable file
 * - asset loading path
 * - threading
 * - raw input (support multiple keyboards)
 * - sleep/timeBeginPeriod
 * - clipcursor (multimonitor support)
 * - fullscreen support
 * - wm_setcursor (control cursor visibility)
 * - querycancelautoplay
 * - wm_activateapp
 * - blit speed improvements
 * - hardware acceleration (opengl/direct3d/vulkan)
 * - getkeyboardlayout
 * */

#include "handmade.cpp"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <x86intrin.h>

#define MAX_CONTROLLERS 4
#define STICK_DEADZONE 8000

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

global_variable bool globalRunning;

global_variable PlatformBackbuffer globalBackbuffer;

global_variable PlatformAudioOutput globalAudioOutput;

global_variable SDL_Gamepad *GamepadHandles[MAX_CONTROLLERS] = {};
global_variable int xOffset = 0, yOffset = 0;

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

void platformInitializeSound(PlatformAudioOutput *audioOutput) {
	audioOutput->sampleRate = 48000;
	audioOutput->numChannels = 2;

	// Set low-latency audio buffer size hint before opening device
	// Lower values = lower latency but more CPU usage
	// At 48000 Hz: 512 samples = ~10.7ms latency, 1024 = ~21.3ms
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "512");

	// Open the audio device
	SDL_AudioSpec spec = {};
	spec.format = SDL_AUDIO_S16;
	spec.channels = audioOutput->numChannels;
	spec.freq = audioOutput->sampleRate;

	audioOutput->device =
		SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
	if (!audioOutput->device) {
		printf("Failed to open audio device: %s\n", SDL_GetError());
		return;
	}

	// Create an audio stream (same format for input and output - no conversion
	// needed)
	SDL_AudioSpec streamSpec = {};
	streamSpec.format = SDL_AUDIO_S16;
	streamSpec.channels = audioOutput->numChannels;
	streamSpec.freq = audioOutput->sampleRate;

	audioOutput->stream = SDL_CreateAudioStream(&streamSpec, &streamSpec);
	if (!audioOutput->stream) {
		printf("Failed to create audio stream: %s\n", SDL_GetError());
		SDL_CloseAudioDevice(audioOutput->device);
		return;
	}

	// Bind the stream to the device
	if (!SDL_BindAudioStream(audioOutput->device, audioOutput->stream)) {
		printf("Failed to bind audio stream: %s\n", SDL_GetError());
		SDL_DestroyAudioStream(audioOutput->stream);
		SDL_CloseAudioDevice(audioOutput->device);
		return;
	}

	// Resume the device (it starts paused)
	SDL_ResumeAudioDevice(audioOutput->device);
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

		bool aButton = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
		if (aButton) {
			yOffset += 2;
			SDL_RumbleGamepad(pad, 20000, 20000, 30);
		} else {
			SDL_RumbleGamepad(pad, 0, 0, 0);
		}
	}
}

int platformGetSamplesToGenerate(int64_t frameStart, int64_t lastFrameStart) {
	// Always generate audio based on elapsed time
	u_int64_t perfFrequency = SDL_GetPerformanceFrequency();
	real64 secondsElapsed =
		(real64)(frameStart - lastFrameStart) / (real64)perfFrequency;

	int samplesToGenerate =
		(int)(secondsElapsed * globalAudioOutput.sampleRate) + 1;

	// Clamp to max one frame at 15fps to handle debugger pauses
	int maxSamplesPerFrame = globalAudioOutput.sampleRate / 15;
	if (samplesToGenerate > maxSamplesPerFrame) {
		samplesToGenerate = maxSamplesPerFrame;
	}

	return samplesToGenerate;
}

bool platformShouldQueueAudioSamples() {

	// Only push to audio stream if queue is below target (prevent
	// accumulation)
	int queued = SDL_GetAudioStreamQueued(globalAudioOutput.stream);
	int queuedSamples =
		queued / (globalAudioOutput.numChannels * sizeof(int16_t));
	// Target: keep about 2-3 frames worth queued (~66ms at 30fps = safe
	// buffer)
	int targetQueuedSamples = (globalAudioOutput.sampleRate / 30) * 2;

	return queuedSamples < targetQueuedSamples;
}

void platformOutputSound(PlatformAudioOutput *audioOutput,
						 GameSoundBuffer *gameSoundBuffer) {
	// Push audio data to the stream
	int bytesPerSample = audioOutput->numChannels * sizeof(int16_t);
	int bytesToWrite = gameSoundBuffer->sampleCount * bytesPerSample;
	SDL_PutAudioStreamData(audioOutput->stream, gameSoundBuffer->samples,
						   bytesToWrite);
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

	platformInitializeSound(&globalAudioOutput);

	globalRunning = true;

	globalBackbuffer = {};
	platformResizeBackbuffer(&globalBackbuffer, renderer, initialWidth,
							 initialHeight);

	int64_t lastFrameStart = SDL_GetPerformanceCounter();

	while (globalRunning) {

		int64_t frameStart = SDL_GetPerformanceCounter();
		uint64_t startCyclesCount = _rdtsc();

		globalRunning = platformProcessEvents(&globalBackbuffer);

		platformProcessControllers();

		GameBackbuffer gamebackbuffer = {};
		gamebackbuffer.width = globalBackbuffer.width;
		gamebackbuffer.height = globalBackbuffer.height;
		gamebackbuffer.pitch = globalBackbuffer.pitch;
		gamebackbuffer.memory = globalBackbuffer.memory;

		// Only generate audio if we're actually going to use it
		if (platformShouldQueueAudioSamples()) {
			GameSoundBuffer gameSoundBuffer = {};
			int16_t samples[48000 * 2]; // 1 second max buffer
			gameSoundBuffer.sampleCount =
				platformGetSamplesToGenerate(frameStart, lastFrameStart);
			gameSoundBuffer.sampleRate = globalAudioOutput.sampleRate;
			gameSoundBuffer.samples = samples;

			gameUpdateAndRender(&gamebackbuffer, &gameSoundBuffer, xOffset,
								yOffset);
			platformOutputSound(&globalAudioOutput, &gameSoundBuffer);
		} else {
			// No audio needed this frame, just render
			GameSoundBuffer gameSoundBuffer = {};
			gameUpdateAndRender(&gamebackbuffer, &gameSoundBuffer, xOffset,
								yOffset);
		}
		platformUpdateWindow(&globalBackbuffer, window, renderer);

		int64_t frameEnd = SDL_GetPerformanceCounter();
		u_int64_t perfFrequency = SDL_GetPerformanceFrequency();
		int64_t frameDuration = frameEnd - frameStart;
		real64 msPerFrame =
			((real64)frameDuration * 1000) / (real64)perfFrequency;
		real64 fps = (real64)perfFrequency / (real64)frameDuration;

		// Recalculate queue for debug display
		int queued = SDL_GetAudioStreamQueued(globalAudioOutput.stream);
		int queuedSamples =
			queued / (globalAudioOutput.numChannels * sizeof(int16_t));
		real64 queuedSeconds =
			(real64)queuedSamples / (real64)globalAudioOutput.sampleRate;

		lastFrameStart = frameStart;

		u_int64_t endCyclesCount = _rdtsc();
		u_int64_t cyclesElapsed = endCyclesCount - startCyclesCount;

		printf("ms/frame: %.02f  fps: %.02f  MegaCycles/frame: %lu  Audio "
			   "queued: %.3fs\n",
			   msPerFrame, fps, cyclesElapsed / (1000 * 1000), queuedSeconds);
	}

	// TODO(bruno): we are not freeing sdl renderer, sdl window and backbuffer
	// here because honestly the OS will handle this for us after this return 0.
	// but maybe we should revisit this?
	// --
	// nah, probably not

	return 0;
}
