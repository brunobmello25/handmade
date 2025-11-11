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

#include <cstddef>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <x86intrin.h>

#include "sdl3_handmade.h"

// TODO(bruno): check deadzone here
// TODO(bruno): go back to episode 19 to improve audio and video sync

#define INPUT_SNAPSHOT_PATH "snapshots/handmade.hmi"
#define MEMORY_SNAPSHOT_PATH "snapshots/handmade.hms"

#ifndef GAME_LIB_PATH
#define GAME_LIB_PATH "handmade.so"
#endif

global_variable bool globalRunning;

global_variable PlatformBackbuffer globalBackbuffer;

global_variable PlatformAudioOutput globalAudioOutput;

global_variable SDL_Gamepad *GamepadHandles[MAX_CONTROLLERS] = {};

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
	backbuffer->memory = calloc(1, width * height * bytesPerPixel);
	backbuffer->texture =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, width, height);
}

void platformStartRecordingInput(PlatformState *platformState,
								 int inputRecordingIndex) {
	assert(platformState->inputPlayingIndex != inputRecordingIndex);
	assert(platformState->inputRecordingIndex == 0);

	platformState->inputRecordingIndex = inputRecordingIndex;

	int handle = open(INPUT_SNAPSHOT_PATH, O_WRONLY | O_CREAT | O_TRUNC,
					  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (handle == -1) {
		assert(!"Failed to open input recording file for writing");
	}

	platformState->inputRecordingHandle = handle;
}

void platformEndRecordingInput(PlatformState *platformState) {
	assert(platformState->inputRecordingIndex != 0);

	close(platformState->inputRecordingHandle);
	platformState->inputRecordingIndex = 0;
}

void platformStartInputPlayback(PlatformState *platformState,
								int playbackIndex) {
	assert(platformState->inputRecordingIndex != playbackIndex);
	assert(platformState->inputPlayingIndex == 0);

	platformState->inputPlayingIndex = playbackIndex;
	int handle = open(INPUT_SNAPSHOT_PATH, O_RDONLY);

	if (handle == -1) {
		assert(!"Failed to open input playback file for reading");
	}

	platformState->inputPlaybackHandle = handle;
}

void platformStopInputPlayback(PlatformState *platformState) {
	assert(platformState->inputPlayingIndex != 0);

	close(platformState->inputPlaybackHandle);
	platformState->inputPlayingIndex = 0;
}

void platformRecordInput(PlatformState platformState, GameInput input) {
	ssize_t bytesToWrite = sizeof(input);
	u_int8_t *nextByteLocation = (u_int8_t *)&input;
	while (bytesToWrite) {
		ssize_t bytesWritten = write(platformState.inputRecordingHandle,
									 nextByteLocation, bytesToWrite);
		if (bytesWritten == -1) {
			return;
		}

		bytesToWrite -= bytesWritten;
		nextByteLocation += bytesWritten;
	}
}

void platformReadMemorySnapshot(void *memory, size_t memorySize, int index) {
	int handle = open(MEMORY_SNAPSHOT_PATH, O_RDONLY);
	if (handle == -1) {
		assert(!"Failed to open memory snapshot file for reading");
	}
	ssize_t bytesToRead = memorySize;
	u_int8_t *nextByteLocation = (u_int8_t *)memory;
	while (bytesToRead) {
		ssize_t bytesRead = read(handle, nextByteLocation, bytesToRead);
		if (bytesRead == -1) {
			close(handle);
			return;
		}
		if (bytesRead == 0) {
			// Reached end of file before reading all expected bytes
			assert(!"Memory snapshot file is smaller than expected");
		}

		bytesToRead -= bytesRead;
		nextByteLocation += bytesRead;
	}
	close(handle);
}

void platformPlaybackInput(PlatformState *platformState, GameInput *input) {
	ssize_t bytesToRead = sizeof(*input);
	u_int8_t *nextByteLocation = (u_int8_t *)input;
	while (bytesToRead) {
		ssize_t bytesRead = read(platformState->inputPlaybackHandle,
								 nextByteLocation, bytesToRead);
		if (bytesRead == 0) {
			lseek(platformState->inputPlaybackHandle, 0, SEEK_SET);
			platformReadMemorySnapshot(platformState->gamePermanentStorage,
									   platformState->permanentStorageSize, 1);
		}
		if (bytesRead == -1) {
			return;
		}

		bytesToRead -= bytesRead;
		nextByteLocation += bytesRead;
	}
}

void platformProcessKeypress(GameButtonState *newState, bool isDown) {
	assert(newState->endedDown != isDown);
	newState->endedDown = isDown;
	newState->halfTransitionCount++;
}

void platformWriteMemorySnapshot(void *memory, size_t memorySize, int index) {
	int handle = open(MEMORY_SNAPSHOT_PATH, O_WRONLY | O_CREAT | O_TRUNC,
					  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (handle == -1) {
		assert(!"Failed to open memory snapshot file for writing");
	}
	ssize_t bytesToWrite = memorySize;
	u_int8_t *nextByteLocation = (u_int8_t *)memory;
	while (bytesToWrite) {
		ssize_t bytesWritten = write(handle, nextByteLocation, bytesToWrite);
		if (bytesWritten == -1) {
			return;
		}

		bytesToWrite -= bytesWritten;
		nextByteLocation += bytesWritten;
	}
	close(handle);
}

bool platformProcessEvents(PlatformBackbuffer *backbuffer,
						   GameControllerInput *keyboardInput, GameInput *input,
						   PlatformState *platformState) {

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

			if (event.key.repeat)
				continue;

			bool isDown = (event.type == SDL_EVENT_KEY_DOWN);

			if (event.key.key == SDLK_W)
				platformProcessKeypress(&keyboardInput->moveUp, isDown);
			if (event.key.key == SDLK_A)
				platformProcessKeypress(&keyboardInput->moveLeft, isDown);
			if (event.key.key == SDLK_S)
				platformProcessKeypress(&keyboardInput->moveDown, isDown);
			if (event.key.key == SDLK_D)
				platformProcessKeypress(&keyboardInput->moveRight, isDown);

			if (event.key.key == SDLK_L && isDown) {
				if (!platformState->inputRecordingIndex &&
					!platformState->inputPlayingIndex) {
					platformStartRecordingInput(platformState, 1);
					platformWriteMemorySnapshot(
						platformState->gamePermanentStorage,
						platformState->permanentStorageSize, 1);
				} else if (platformState->inputRecordingIndex) {
					platformEndRecordingInput(platformState);
					platformReadMemorySnapshot(
						platformState->gamePermanentStorage,
						platformState->permanentStorageSize, 1);
					platformStartInputPlayback(platformState, 1);
				} else if (platformState->inputPlayingIndex) {
					platformStopInputPlayback(platformState);
				} else {
					// TODO(bruno): probably want to handle multiple playback
					// indexes here
					assert(!"Impossible state in input recording/playback");
				}
			}
		}
		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
			event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
			bool isDown = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);

			if (event.button.button == SDL_BUTTON_LEFT) {
				platformProcessKeypress(&input->mouseButtons[0], isDown);
			}
			if (event.button.button == SDL_BUTTON_MIDDLE) {
				platformProcessKeypress(&input->mouseButtons[1], isDown);
			}
			if (event.button.button == SDL_BUTTON_RIGHT) {
				platformProcessKeypress(&input->mouseButtons[2], isDown);
			}
			if (event.button.button == SDL_BUTTON_X1) {
				platformProcessKeypress(&input->mouseButtons[3], isDown);
			}
			if (event.button.button == SDL_BUTTON_X2) {
				platformProcessKeypress(&input->mouseButtons[4], isDown);
			}
		}
		if (event.type == SDL_EVENT_MOUSE_MOTION) {
			input->mouseX = event.motion.x;
			input->mouseY = event.motion.y;
		}
		if (event.type == SDL_EVENT_MOUSE_WHEEL) {
			input->mouseZ += (int32_t)event.wheel.y;
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

#if HANDMADE_INTERNAL
void DEBUGplatformDrawDebugAudioLine(PlatformBackbuffer *buffer, int x, int top,
									 int bottom, uint32_t color) {
	if (x < 0 || x >= buffer->width)
		return;

	uint32_t *pixel = (uint32_t *)buffer->memory;
	for (int y = top; y < bottom && y < buffer->height; y++) {
		pixel[y * buffer->width + x] = color;
	}
}

void DEBUGPlatformDrawDebugAudio(GameSoundBuffer *gameSoundBuffer) {
	// Draw debug audio visualization
	// Shows audio buffer state before we update the window
	int debugQueued = SDL_GetAudioStreamQueued(globalAudioOutput.stream);
	int debugQueuedSamples =
		debugQueued / (globalAudioOutput.numChannels * sizeof(int16_t));
	int targetQueuedSamples = (globalAudioOutput.sampleRate / 30) * 2;

	// Draw a visual representation at the top of the screen
	// White line = current frame position
	// Green area = queued audio
	// Red line = target queue level
	// Yellow area = audio we just generated this frame

	int debugHeight = 20;
	int debugTop = 10;

	// Scale: 1 pixel = (sampleRate / width) samples
	real32 samplesPerPixel =
		(real32)(globalAudioOutput.sampleRate) / (real32)globalBackbuffer.width;

	// Draw current queued level (green)
	int queuedPixels = (int)((real32)debugQueuedSamples / samplesPerPixel);
	for (int x = 0; x < queuedPixels && x < globalBackbuffer.width; x++) {
		DEBUGplatformDrawDebugAudioLine(&globalBackbuffer, x, debugTop,
										debugTop + debugHeight, 0xFF00FF00);
	}

	// Draw target queue level (red vertical line)
	int targetPixels = (int)((real32)targetQueuedSamples / samplesPerPixel);
	DEBUGplatformDrawDebugAudioLine(&globalBackbuffer, targetPixels, debugTop,
									debugTop + debugHeight + 5, 0xFFFF0000);

	// Draw current frame's audio generation (yellow, on top of green)
	if (gameSoundBuffer->sampleCount > 0) {
		int generatedPixels =
			(int)((real32)gameSoundBuffer->sampleCount / samplesPerPixel);
		for (int x = queuedPixels - generatedPixels;
			 x < queuedPixels && x >= 0 && x < globalBackbuffer.width; x++) {
			DEBUGplatformDrawDebugAudioLine(&globalBackbuffer, x, debugTop,
											debugTop + debugHeight, 0xFFFFFF00);
		}
	}

	// Draw frame marker (white vertical line at beginning)
	DEBUGplatformDrawDebugAudioLine(&globalBackbuffer, 0, debugTop - 5,
									debugTop + debugHeight + 5, 0xFFFFFFFF);
}

DEBUGReadFileResult DEBUGPlatformReadEntireFile(const char *filename) {
	DEBUGReadFileResult result = {};
	int handle = open(filename, O_RDONLY);
	if (handle == -1) {
		return result;
	}

	struct stat status;
	if (fstat(handle, &status) == -1) {
		close(handle);
		return result;
	}

	result.size = safeTruncateUint64(status.st_size);

	result.data = malloc(result.size);
	if (!result.data) {
		close(handle);
		result.size = 0;
		return result;
	}

	ssize_t bytesToRead = result.size;
	u_int8_t *nextByteLocation = (u_int8_t *)result.data;
	while (bytesToRead) {
		ssize_t bytesRead = read(handle, nextByteLocation, bytesToRead);
		if (bytesRead == -1) {
			free(result.data);
			result.data = 0;
			result.size = 0;
			close(handle);
			return result;
		}

		bytesToRead -= bytesRead;
		nextByteLocation += bytesRead;
	}

	close(handle);
	return result;
}

void DEBUGPlatformFreeFileMemory(void *memory) { free(memory); }

bool DEBUGPlatformWriteEntireFile(const char *filename, u_int32_t size,
								  void *memory) {
	int handle = open(filename, O_WRONLY | O_CREAT,
					  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (handle == -1) {
		return false;
	}

	ssize_t bytesToWrite = size;
	u_int8_t *nextByteLocation = (u_int8_t *)memory;
	while (bytesToWrite) {
		ssize_t bytesWritten = write(handle, nextByteLocation, bytesToWrite);
		if (bytesWritten == -1) {
			close(handle);
			return false;
		}

		bytesToWrite -= bytesWritten;
		nextByteLocation += bytesWritten;
	}

	close(handle);
	return true;
}
#endif

void platformUpdateWindow(PlatformBackbuffer *buffer, SDL_Window *window,
						  SDL_Renderer *renderer) {
	SDL_UpdateTexture(buffer->texture, NULL, buffer->memory, buffer->pitch);
	SDL_RenderTexture(renderer, buffer->texture, 0, 0);
	SDL_RenderPresent(renderer);
}

void platformLoadControllers() {

	int gamepadCount;
	uint *ids = SDL_GetGamepads(&gamepadCount);

	// TODO: handle proper controller count exceeding the size of game
	// controllers (which is currently 4)
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

void processControllerButton(GameButtonState *oldState,
							 GameButtonState *newState, bool value) {
	newState->endedDown = value;
	newState->halfTransitionCount +=
		(oldState->endedDown != newState->endedDown) ? 1 : 0;
}

void platformProcessControllers(GameInput *gameInput) {

	// TODO(bruno): handle this loop when we have more controllers on sdl than
	// on game
	for (int i = 0; i < MAX_CONTROLLERS; i++) {
		GameControllerInput *oldController = &gameInput->controllers[i + 1];
		GameControllerInput *newController = &gameInput->controllers[i + 1];

		SDL_Gamepad *pad = GamepadHandles[i];
		if (!pad) {
			newController->isConnected = false;
			continue;
		}

		newController->isConnected = true;

		// Process D-pad buttons
		bool dpadUp = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP);
		bool dpadDown = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
		bool dpadLeft = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
		bool dpadRight =
			SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

		processControllerButton(&oldController->moveUp, &newController->moveUp,
								dpadUp);
		processControllerButton(&oldController->moveDown,
								&newController->moveDown, dpadDown);
		processControllerButton(&oldController->moveLeft,
								&newController->moveLeft, dpadLeft);
		processControllerButton(&oldController->moveRight,
								&newController->moveRight, dpadRight);

		int16_t sdlStickX = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
		int16_t sdlStickY = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);
		real32 stickX;
		if (sdlStickX < 0) {
			stickX = (real32)sdlStickX / 32768.0f;
		} else {
			stickX = (real32)sdlStickX / 32767.0f;
		}
		real32 stickY;
		if (sdlStickY < 0) {
			stickY = (real32)sdlStickY / 32768.0f;
		} else {
			stickY = (real32)sdlStickY / 32767.0f;
		}

		newController->stickAverageX = stickX;
		newController->stickAverageY = stickY;

		// Set isAnalog based on stick movement and dpad state
		// If any dpad button is pressed, set to false (digital input)
		// Otherwise, set based on whether stick is being used
		bool dpadPressed = dpadUp || dpadDown || dpadLeft || dpadRight;
		if (dpadPressed) {
			newController->isAnalog = false;
		} else {
			newController->isAnalog = (stickX != 0.0f || stickY != 0.0f);
		}

		processControllerButton(
			&oldController->actionDown, &newController->actionDown,
			SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH));
		processControllerButton(
			&oldController->actionRight, &newController->actionRight,
			SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST));
		processControllerButton(
			&oldController->actionLeft, &newController->actionLeft,
			SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST));
		processControllerButton(
			&oldController->actionUp, &newController->actionUp,
			SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH));

		// TODO(bruno): rumble
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

bool platformInitializeGameMemory(GameMemory *gameMemory,
								  PlatformState *platformState) {
	gameMemory->permanentStorageSize = Megabytes(64);
	gameMemory->transientStorageSize = Gigabytes(4);

	size_t totalSize =
		gameMemory->permanentStorageSize + gameMemory->transientStorageSize;

#if HANDMADE_INTERNAL
	void *baseAddress = (void *)Terabytes(2);
#else
	void *baseAddress = NULL;
#endif

	void *memory = mmap(baseAddress, totalSize, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (memory == MAP_FAILED) {
		printf("Failed to allocate game memory\n");
		return false;
	}

	gameMemory->permanentStorage = memory;
	gameMemory->transientStorage =
		(void *)((uint8_t *)memory + gameMemory->permanentStorageSize);

	gameMemory->DEBUGPlatformReadEntireFile = &DEBUGPlatformReadEntireFile;
	gameMemory->DEBUGPlatformFreeFileMemory = &DEBUGPlatformFreeFileMemory;
	gameMemory->DEBUGPlatformWriteEntireFile = &DEBUGPlatformWriteEntireFile;

	platformState->gamePermanentStorage = memory;
	platformState->permanentStorageSize = totalSize;
	platformState->permanentStorageSize = gameMemory->permanentStorageSize;

	return true;
}

void platformOutputSound(PlatformAudioOutput *audioOutput,
						 GameSoundBuffer *gameSoundBuffer) {
	if (!gameSoundBuffer->samples || gameSoundBuffer->sampleCount == 0)
		return;
	// Push audio data to the stream
	int bytesPerSample = audioOutput->numChannels * sizeof(int16_t);
	int bytesToWrite = gameSoundBuffer->sampleCount * bytesPerSample;
	SDL_PutAudioStreamData(audioOutput->stream, gameSoundBuffer->samples,
						   bytesToWrite);
}

int platformGetDisplayRefreshRate(SDL_Window *window) {
	int defaultRefreshRate = 60;

	SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
	if (!displayID) {
		return defaultRefreshRate;
	}

	const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(displayID);

	if (!mode || mode->refresh_rate <= 0) {
		return defaultRefreshRate;
	}

	return mode->refresh_rate;
}

real32 platformGetSecondsElapsed(int64_t oldCounter, int64_t currentCounter) {
	return ((real32)(currentCounter - oldCounter) /
			(real32)SDL_GetPerformanceFrequency());
}

inline void platformDelayFrame(int64_t frameStart,
							   real32 targetSecondsPerFrame) {

	real32 secondsElapsed =
		platformGetSecondsElapsed(frameStart, SDL_GetPerformanceCounter());

	if (secondsElapsed < targetSecondsPerFrame) {
		u_int32_t msToSleep =
			(u_int32_t)((targetSecondsPerFrame - secondsElapsed) * 1000.0f);
		if (msToSleep > 0) {
			SDL_Delay(msToSleep);
		}
	}
}

time_t platformGetFileModTime(const char *filename) {
	struct stat fileStatus;
	if (stat(filename, &fileStatus) == 0) {
		return fileStatus.st_mtime;
	}
	return 0;
}

void platformUnloadGameCode(PlatformGameCode *platformGameCode) {
	if (platformGameCode->loaded) {
		dlclose(platformGameCode->gameLib);
		platformGameCode->loaded = false;
		platformGameCode->gameLib = NULL;
		platformGameCode->gameUpdateAndRender = NULL;
	}
}

bool platformLoadGameCode(PlatformGameCode *platformGameCode) {
	platformGameCode->lastModTime = platformGetFileModTime(GAME_LIB_PATH);

	platformGameCode->gameLib = dlopen(GAME_LIB_PATH, RTLD_LAZY);
	if (!platformGameCode->gameLib)
		return false;

	platformGameCode->gameUpdateAndRender = (GAME_UPDATE_AND_RENDER)dlsym(
		platformGameCode->gameLib, "gameUpdateAndRender");

	if (!platformGameCode->gameUpdateAndRender) {
		printf("Failed to load gameUpdateAndRender: %s\n", dlerror());
		dlclose(platformGameCode->gameLib);
		return false;
	}

	platformGameCode->loaded = true;

	return true;
}

int main(void) {
	int initialWidth = 1920 / 2;
	int initialHeight = 1080 / 2;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD |
				  SDL_INIT_AUDIO))
		return -1;

	platformLoadControllers();
	GameInput gameInputs[2];
	gameInputs[0] = {};
	gameInputs[1] = {};
	GameInput *newInput = &gameInputs[0];
	GameInput *oldInput = &gameInputs[1];

	SDL_Window *window = SDL_CreateWindow("Handmade Hero", initialWidth,
										  initialHeight, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
	if (!window || !renderer) // TODO(bruno): proper error handling
		return -1;
	if (!SDL_SetRenderVSync(renderer, 1)) {
		printf("Failed to set vsync on renderer: %s\n", SDL_GetError());
	}

	int refreshRate = platformGetDisplayRefreshRate(window);
	real32 targetSecondsPerFrame =
		1.0f / (real32)refreshRate; // TODO(bruno): change the game refresh rate
									// based on the hardware capacity: if it's
									// too slow, lower it to 30fps

	GameMemory gameMemory = {};
	PlatformState platformState = {};

	if (!platformInitializeGameMemory(&gameMemory, &platformState)) {
		return -1; // TODO(bruno): proper error handling
	}

	platformInitializeSound(&globalAudioOutput);

	globalRunning = true;

	globalBackbuffer = {};
	platformResizeBackbuffer(&globalBackbuffer, renderer, initialWidth,
							 initialHeight);

	int64_t lastFrameStart = SDL_GetPerformanceCounter();

	PlatformGameCode gameCode = {};
	platformLoadGameCode(&gameCode);

	while (globalRunning) {
		time_t newModTime = platformGetFileModTime(GAME_LIB_PATH);
		if (newModTime != gameCode.lastModTime) {
			platformUnloadGameCode(&gameCode);
			platformLoadGameCode(&gameCode);
		}

		int64_t frameStart = SDL_GetPerformanceCounter();
		uint64_t startCyclesCount = _rdtsc();

		platformProcessControllers(newInput);
		GameInput *temp = oldInput;
		oldInput = newInput;
		newInput = temp;
		newInput->deltaTime = targetSecondsPerFrame; // TODO(bruno): calculate
													 // real delta time

		GameControllerInput *oldKeyboard = &oldInput->controllers[0];
		GameControllerInput *newKeyboard = &newInput->controllers[0];
		*newKeyboard = {};
		newKeyboard->isConnected = true;
		for (size_t i = 0; i < arraylength(newKeyboard->buttons); i++) {
			newKeyboard->buttons[i].endedDown =
				oldKeyboard->buttons[i].endedDown;
		}

		// Preserve mouse button state across frames
		for (size_t i = 0; i < arraylength(newInput->mouseButtons); i++) {
			newInput->mouseButtons[i].endedDown =
				oldInput->mouseButtons[i].endedDown;
		}
		newInput->mouseX = oldInput->mouseX;
		newInput->mouseY = oldInput->mouseY;
		newInput->mouseZ = oldInput->mouseZ;

		globalRunning = platformProcessEvents(&globalBackbuffer, newKeyboard,
											  newInput, &platformState);

		GameBackbuffer gamebackbuffer = {};
		gamebackbuffer.width = globalBackbuffer.width;
		gamebackbuffer.height = globalBackbuffer.height;
		gamebackbuffer.pitch = globalBackbuffer.pitch;
		gamebackbuffer.memory = globalBackbuffer.memory;

		// Only generate audio if we're actually going to use it
		GameSoundBuffer gameSoundBuffer = {};
		int16_t samples[48000 * 2]; // TODO(bruno): 1 second max buffer.
		if (platformShouldQueueAudioSamples()) {
			// make this dynamic later
			gameSoundBuffer.sampleCount =
				platformGetSamplesToGenerate(frameStart, lastFrameStart);
			gameSoundBuffer.sampleRate = globalAudioOutput.sampleRate;
			gameSoundBuffer.samples = samples;
		}

		if (platformState.inputRecordingIndex) {
			platformRecordInput(platformState, *newInput);
		}
		if (platformState.inputPlayingIndex) {
			platformPlaybackInput(&platformState, newInput);
		}

		gameCode.gameUpdateAndRender(&gameMemory, &gamebackbuffer,
									 &gameSoundBuffer, newInput);
		platformOutputSound(&globalAudioOutput, &gameSoundBuffer);

#if HANDMADE_PLATFORMDEBUG
		DEBUGPlatformDrawDebugAudio(&gameSoundBuffer);
#endif

		platformUpdateWindow(&globalBackbuffer, window, renderer);

		platformDelayFrame(frameStart, targetSecondsPerFrame);

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

#if HANDMADE_PLATFORMDEBUG
		printf("ms/frame: %.02f  fps: %.02f  MegaCycles/frame: %lu  Audio "
			   "queued: %.3fs\n",
			   msPerFrame, fps, cyclesElapsed / (1000 * 1000), queuedSeconds);
#endif
	}

	// TODO(bruno): we are not freeing sdl renderer, sdl window and backbuffer
	// here because honestly the OS will handle this for us after this return 0.
	// but maybe we should revisit this?
	// --
	// nah, probably not

	return 0;
}
