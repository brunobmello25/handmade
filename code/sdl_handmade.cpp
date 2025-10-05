#include <stdint.h>
#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

// TODO: Implement sine ourselved
#include <math.h>

#include "handmade.cpp"

#include <SDL.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <x86intrin.h>

#include "sdl_handmade.h"

// NOTE: MAP_ANONYMOUS is not defined on Mac OS X and some other UNIX systems.
// On the vast majority of those systems, one can use MAP_ANON instead.
// Huge thanks to Adam Rosenfield for investigating this, and suggesting this
// workaround:
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

global_variable SdlOffscreenBuffer GlobalBackbuffer;

#define MAX_CONTROLLERS 4
SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

SdlAudioRingBuffer AudioRingBuffer;

internal DebugReadFileResult DEBUGPlatformReadEntireFile(char *filename)
{
	DebugReadFileResult result = {};

	int fileHandle = open(filename, O_RDONLY);
	if (fileHandle == -1)
	{
		return result;
	}

	struct stat fileStatus;
	if (fstat(fileHandle, &fileStatus) == -1)
	{
		close(fileHandle);
		return result;
	}
	result.contentsSize = SafeTruncateUInt64(fileStatus.st_size);

	result.contents = malloc(result.contentsSize);
	if (!result.contents)
	{
		close(fileHandle);
		result.contentsSize = 0;
		return result;
	}

	uint32 bytesToRead = result.contentsSize;
	uint8 *nextByteLocation = (uint8 *)result.contents;
	while (bytesToRead)
	{
		ssize_t bytesRead = read(fileHandle, nextByteLocation, bytesToRead);
		if (bytesRead == -1)
		{
			free(result.contents);
			result.contents = 0;
			result.contentsSize = 0;
			close(fileHandle);
			return result;
		}
		bytesToRead -= bytesRead;
		nextByteLocation += bytesRead;
	}

	close(fileHandle);
	return (result);
}

internal void DEBUGPlatformFreeFileMemory(void *memory) { free(memory); }

internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize,
											 void *memory)
{
	int fileHandle = open(filename, O_WRONLY | O_CREAT,
						  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (!fileHandle)
		return false;

	uint32 bytesToWrite = memorySize;
	uint8 *nextByteLocation = (uint8 *)memory;
	while (bytesToWrite)
	{
		ssize_t bytesWritten =
			write(fileHandle, nextByteLocation, bytesToWrite);
		if (bytesWritten == -1)
		{
			close(fileHandle);
			return false;
		}
		bytesToWrite -= bytesWritten;
		nextByteLocation += bytesWritten;
	}

	close(fileHandle);

	return true;
}

internal void SDLAudioCallback(void *userData, Uint8 *audioData, int length)
{
	SdlAudioRingBuffer *ringBuffer = (SdlAudioRingBuffer *)userData;

	int region1Size = length;
	int region2Size = 0;
	if (ringBuffer->playCursor + length > ringBuffer->size)
	{
		region1Size = ringBuffer->size - ringBuffer->playCursor;
		region2Size = length - region1Size;
	}
	memcpy(audioData, (uint8 *)(ringBuffer->data) + ringBuffer->playCursor,
		   region1Size);
	memcpy(&audioData[region1Size], ringBuffer->data, region2Size);
	ringBuffer->playCursor =
		(ringBuffer->playCursor + length) % ringBuffer->size;
	ringBuffer->writeCursor =
		(ringBuffer->playCursor + length) % ringBuffer->size;
}

internal void SDLInitAudio(int32 samplesPerSecond, int32 bufferSize)
{
	SDL_AudioSpec audioSettings = {0};

	audioSettings.freq = samplesPerSecond;
	audioSettings.format = AUDIO_S16LSB;
	audioSettings.channels = 2;
	audioSettings.samples = 512;
	audioSettings.callback = &SDLAudioCallback;
	audioSettings.userdata = &AudioRingBuffer;

	AudioRingBuffer.size = bufferSize;
	AudioRingBuffer.data = calloc(bufferSize, 1);
	AudioRingBuffer.playCursor = AudioRingBuffer.writeCursor = 0;

	SDL_OpenAudio(&audioSettings, 0);

	printf("Initialised an Audio device at frequency %d Hz, %d Channels, "
		   "buffer size %d\n",
		   audioSettings.freq, audioSettings.channels, audioSettings.size);

	if (audioSettings.format != AUDIO_S16LSB)
	{
		printf("Oops! We didn't get AUDIO_S16LSB as our sample format!\n");
		SDL_CloseAudio();
	}
}

SdlWindowDimension SDLGetWindowDimension(SDL_Window *window)
{
	SdlWindowDimension result;

	SDL_GetWindowSize(window, &result.width, &result.height);

	return (result);
}

internal void SDLResizeTexture(SdlOffscreenBuffer *buffer,
							   SDL_Renderer *renderer, int width, int height)
{
	int bytesPerPixel = 4;
	if (buffer->memory)
	{
		munmap(buffer->memory, buffer->width * buffer->height * bytesPerPixel);
	}
	if (buffer->texture)
	{
		SDL_DestroyTexture(buffer->texture);
	}
	buffer->texture =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, width, height);
	buffer->width = width;
	buffer->height = height;
	buffer->pitch = width * bytesPerPixel;
	buffer->memory =
		mmap(0, width * height * bytesPerPixel, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

internal void SDLUpdateWindow(SDL_Window *window, SDL_Renderer *renderer,
							  SdlOffscreenBuffer *buffer)
{
	SDL_UpdateTexture(buffer->texture, 0, buffer->memory, buffer->pitch);

	SDL_RenderCopy(renderer, buffer->texture, 0, 0);

	SDL_RenderPresent(renderer);
}

internal void SDLProcessKeyPress(GameButtonState *newState, bool32 isDown)
{
	assert(newState->endedDown != isDown);
	newState->endedDown = isDown;
	++newState->halfTransitionCount;
}

bool HandleEvent(SDL_Event *event, GameControllerInput *newKeyboardController)
{
	bool shouldQuit = false;

	switch (event->type)
	{
		case SDL_QUIT:
		{
			printf("SDL_QUIT\n");
			shouldQuit = true;
		}
		break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			SDL_Keycode keyCode = event->key.keysym.sym;
			bool isDown = (event->key.state == SDL_PRESSED);
			bool wasDown = false;
			if (event->key.state == SDL_RELEASED)
			{
				wasDown = true;
			}
			else if (event->key.repeat != 0)
			{
				wasDown = true;
			}

			// NOTE: In the windows version, we used "if (isDown != wasDown)"
			// to detect key repeats. SDL has the 'repeat' value, though,
			// which we'll use.
			if (event->key.repeat == 0)
			{
				if (keyCode == SDLK_w)
				{
					SDLProcessKeyPress(&newKeyboardController->moveUp, isDown);
				}
				else if (keyCode == SDLK_a)
				{
					SDLProcessKeyPress(&newKeyboardController->moveLeft,
									   isDown);
				}
				else if (keyCode == SDLK_s)
				{
					SDLProcessKeyPress(&newKeyboardController->moveDown,
									   isDown);
				}
				else if (keyCode == SDLK_d)
				{
					SDLProcessKeyPress(&newKeyboardController->moveRight,
									   isDown);
				}
				else if (keyCode == SDLK_q)
				{
					SDLProcessKeyPress(&newKeyboardController->leftShoulder,
									   isDown);
				}
				else if (keyCode == SDLK_e)
				{
					SDLProcessKeyPress(&newKeyboardController->rightShoulder,
									   isDown);
				}
				else if (keyCode == SDLK_UP)
				{
					SDLProcessKeyPress(&newKeyboardController->actionUp,
									   isDown);
				}
				else if (keyCode == SDLK_LEFT)
				{
					SDLProcessKeyPress(&newKeyboardController->actionLeft,
									   isDown);
				}
				else if (keyCode == SDLK_DOWN)
				{
					SDLProcessKeyPress(&newKeyboardController->actionDown,
									   isDown);
				}
				else if (keyCode == SDLK_RIGHT)
				{
					SDLProcessKeyPress(&newKeyboardController->actionRight,
									   isDown);
				}
				else if (keyCode == SDLK_ESCAPE)
				{
					printf("ESCAPE: ");
					if (isDown)
					{
						printf("isDown ");
					}
					if (wasDown)
					{
						printf("wasDown");
					}
					printf("\n");
				}
				else if (keyCode == SDLK_SPACE)
				{
				}
			}

			bool altKeyWasDown = (event->key.keysym.mod & KMOD_ALT);
			if (keyCode == SDLK_F4 && altKeyWasDown)
			{
				shouldQuit = true;
			}
		}
		break;

		case SDL_WINDOWEVENT:
		{
			switch (event->window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n",
						   event->window.data1, event->window.data2);
				}
				break;

				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
				}
				break;

				case SDL_WINDOWEVENT_EXPOSED:
				{
					SDL_Window *window =
						SDL_GetWindowFromID(event->window.windowID);
					SDL_Renderer *renderer = SDL_GetRenderer(window);
					SDLUpdateWindow(window, renderer, &GlobalBackbuffer);
				}
				break;
			}
		}
		break;
	}

	return (shouldQuit);
}

internal void SDLFillSoundBuffer(SdlSoundOutput *soundOutput, int byteToLock,
								 int bytesToWrite,
								 GameSoundOutputBuffer *soundBuffer)
{
	int16_t *samples = soundBuffer->samples;
	void *region1 = (uint8 *)AudioRingBuffer.data + byteToLock;
	int region1Size = bytesToWrite;
	if (region1Size + byteToLock > soundOutput->secondaryBufferSize)
	{
		region1Size = soundOutput->secondaryBufferSize - byteToLock;
	}
	void *region2 = AudioRingBuffer.data;
	int region2Size = bytesToWrite - region1Size;
	int region1SampleCount = region1Size / soundOutput->bytesPerSample;
	int16 *sampleOut = (int16 *)region1;
	for (int sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
	{
		*sampleOut++ = *samples++;
		*sampleOut++ = *samples++;

		++soundOutput->runningSampleIndex;
	}

	int region2SampleCount = region2Size / soundOutput->bytesPerSample;
	sampleOut = (int16 *)region2;
	for (int sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
	{
		*sampleOut++ = *samples++;
		*sampleOut++ = *samples++;
		++soundOutput->runningSampleIndex;
	}
}

internal void SDLOpenGameControllers()
{
	int maxJoysticks = SDL_NumJoysticks();
	int controllerIndex = 0;
	for (int joystickIndex = 0; joystickIndex < maxJoysticks; ++joystickIndex)
	{
		if (!SDL_IsGameController(joystickIndex))
		{
			continue;
		}
		if (controllerIndex >= MAX_CONTROLLERS)
		{
			break;
		}
		ControllerHandles[controllerIndex] =
			SDL_GameControllerOpen(joystickIndex);
		SDL_Joystick *joystickHandle =
			SDL_GameControllerGetJoystick(ControllerHandles[controllerIndex]);
		RumbleHandles[controllerIndex] =
			SDL_HapticOpenFromJoystick(joystickHandle);
		if (SDL_HapticRumbleInit(RumbleHandles[controllerIndex]) != 0)
		{
			SDL_HapticClose(RumbleHandles[controllerIndex]);
			RumbleHandles[controllerIndex] = 0;
		}

		controllerIndex++;
	}
}

internal void SDLProcessGameControllerButton(GameButtonState *oldState,
											 GameButtonState *newState,
											 bool value)
{
	newState->endedDown = value;
	newState->halfTransitionCount +=
		((newState->endedDown == oldState->endedDown) ? 0 : 1);
}

internal real32 SDLProcessGameControllerAxisValue(int16 value,
												  int16 deadZoneThreshold)
{
	real32 result = 0;

	if (value < -deadZoneThreshold)
	{
		result = (real32)((value + deadZoneThreshold) /
						  (32768.0f - deadZoneThreshold));
	}
	else if (value > deadZoneThreshold)
	{
		result = (real32)((value - deadZoneThreshold) /
						  (32767.0f - deadZoneThreshold));
	}

	return (result);
}

internal void SDLCloseGameControllers()
{
	for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
		 ++controllerIndex)
	{
		if (ControllerHandles[controllerIndex])
		{
			if (RumbleHandles[controllerIndex])
				SDL_HapticClose(RumbleHandles[controllerIndex]);
			SDL_GameControllerClose(ControllerHandles[controllerIndex]);
		}
	}
}

internal int SDLGetWindowRefreshRate(SDL_Window *window)
{
	int displayIndex = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode mode;
	int defaultRefreshRate = 60;

	if (SDL_GetDesktopDisplayMode(displayIndex, &mode) != 0)
	{
		return defaultRefreshRate;
	}

	if (mode.refresh_rate == 0)
	{
		return defaultRefreshRate;
	}

	return mode.refresh_rate;
}

internal real32 SDLGetSecondsElapsed(uint64 oldCounter, uint64 currentCounter)
{
	return ((real32)(currentCounter - oldCounter) /
			(real32)SDL_GetPerformanceFrequency());
}
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC |
			 SDL_INIT_AUDIO);
	uint64 perfCountFrequency = SDL_GetPerformanceFrequency();
	// Initialise our Game Controllers:
	SDLOpenGameControllers();
	// Create our window.
	SDL_Window *window = SDL_CreateWindow(
		"Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640,
		480, SDL_WINDOW_RESIZABLE);
	if (window)
	{
		// Create a "renderer" for our window.
		SDL_Renderer *renderer =
			SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

		int monitorRefreshRate = SDLGetWindowRefreshRate(window);
		int gameUpdateHz = monitorRefreshRate / 2;
		real32 targetSecondsPerFrame = 1.0f / (real32)gameUpdateHz;

		if (renderer)
		{
			bool running = true;
			SdlWindowDimension dimension = SDLGetWindowDimension(window);
			SDLResizeTexture(&GlobalBackbuffer, renderer, dimension.width,
							 dimension.height);

			GameInput input[2] = {};
			GameInput *newInput = &input[0];
			GameInput *oldInput = &input[1];

			SdlSoundOutput soundOutput = {};
			soundOutput.samplesPerSecond = 48000;
			soundOutput.runningSampleIndex = 0;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.secondaryBufferSize =
				soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
			// Open our audio device:
			SDLInitAudio(48000, soundOutput.secondaryBufferSize);
			// NOTE: calloc() allocates memory and clears it to zero. It accepts
			// the number of things being allocated and their size.
			int16 *samples = (int16 *)calloc(soundOutput.samplesPerSecond,
											 soundOutput.bytesPerSample);
			SDL_PauseAudio(0);

#if HANDMADE_INTERNAL
			void *baseAddress = (void *)Terabytes(2);
#else
			void *baseAddress = (void *)(0);
#endif

			GameMemory gameMemory = {};
			gameMemory.permanentStorageSize = Megabytes(64);
			gameMemory.transientStorageSize = Gigabytes(4);

			uint64 totalStorageSize = gameMemory.permanentStorageSize +
									  gameMemory.transientStorageSize;

			gameMemory.permanentStorage =
				mmap(baseAddress, totalStorageSize, PROT_READ | PROT_WRITE,
					 MAP_ANON | MAP_PRIVATE, -1, 0);

			assert(gameMemory.permanentStorage);

			gameMemory.transientStorage =
				(uint8 *)(gameMemory.permanentStorage) +
				gameMemory.permanentStorageSize;

			uint64 lastCounter = SDL_GetPerformanceCounter();
			uint64 lastCycleCount = _rdtsc();
			while (running)
			{
				GameControllerInput *oldKeyboardController =
					GetController(oldInput, 0);
				GameControllerInput *newKeyboardController =
					GetController(newInput, 0);
				*newKeyboardController = {};
				for (size_t buttonIndex = 0;
					 buttonIndex < ArrayCount(newKeyboardController->buttons);
					 ++buttonIndex)
				{
					newKeyboardController->buttons[buttonIndex].endedDown =
						oldKeyboardController->buttons[buttonIndex].endedDown;
				}
				SDL_Event event;
				while (SDL_PollEvent(&event))
				{
					if (HandleEvent(&event, newKeyboardController))
					{
						running = false;
					}
				}

				// Poll our controllers for input.
				for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
					 ++controllerIndex)
				{
					if (ControllerHandles[controllerIndex] != 0 &&
						SDL_GameControllerGetAttached(
							ControllerHandles[controllerIndex]))
					{
						GameControllerInput *oldController =
							GetController(oldInput, controllerIndex + 1);
						GameControllerInput *newController =
							GetController(newInput, controllerIndex + 1);

						newController->isConnected = true;

						// TODO: Do something with the DPad, start and Selected?
						bool up = SDL_GameControllerGetButton(
							ControllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_UP);
						bool down = SDL_GameControllerGetButton(
							ControllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_DOWN);
						bool left = SDL_GameControllerGetButton(
							ControllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_LEFT);
						bool right = SDL_GameControllerGetButton(
							ControllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
						bool start = SDL_GameControllerGetButton(
							ControllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_START);
						bool back = SDL_GameControllerGetButton(
							ControllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_BACK);

						SDLProcessGameControllerButton(
							&(oldController->leftShoulder),
							&(newController->leftShoulder),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_LEFTSHOULDER));

						SDLProcessGameControllerButton(
							&(oldController->rightShoulder),
							&(newController->rightShoulder),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

						SDLProcessGameControllerButton(
							&(oldController->actionDown),
							&(newController->actionDown),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_A));

						SDLProcessGameControllerButton(
							&(oldController->actionRight),
							&(newController->actionRight),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_B));

						SDLProcessGameControllerButton(
							&(oldController->actionLeft),
							&(newController->actionLeft),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_X));

						SDLProcessGameControllerButton(
							&(oldController->actionUp),
							&(newController->actionUp),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_Y));

						newController->stickAverageX =
							SDLProcessGameControllerAxisValue(
								SDL_GameControllerGetAxis(
									ControllerHandles[controllerIndex],
									SDL_CONTROLLER_AXIS_LEFTX),
								1);
						newController->stickAverageY =
							-SDLProcessGameControllerAxisValue(
								SDL_GameControllerGetAxis(
									ControllerHandles[controllerIndex],
									SDL_CONTROLLER_AXIS_LEFTY),
								1);
						if ((newController->stickAverageX != 0.0f) ||
							(newController->stickAverageY != 0.0f))
						{
							newController->isAnalog = true;
						}

						if (SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_DPAD_UP))
						{
							newController->stickAverageY = 1.0f;
							newController->isAnalog = false;
						}

						if (SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_DPAD_DOWN))
						{
							newController->stickAverageY = -1.0f;
							newController->isAnalog = false;
						}

						if (SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_DPAD_LEFT))
						{
							newController->stickAverageX = -1.0f;
							newController->isAnalog = false;
						}

						if (SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
						{
							newController->stickAverageX = 1.0f;
							newController->isAnalog = false;
						}

						real32 threshold = 0.5f;
						SDLProcessGameControllerButton(
							&(oldController->moveLeft),
							&(newController->moveLeft),
							newController->stickAverageX < -threshold);
						SDLProcessGameControllerButton(
							&(oldController->moveRight),
							&(newController->moveRight),
							newController->stickAverageX > threshold);
						SDLProcessGameControllerButton(
							&(oldController->moveUp), &(newController->moveUp),
							newController->stickAverageY < -threshold);
						SDLProcessGameControllerButton(
							&(oldController->moveDown),
							&(newController->moveDown),
							newController->stickAverageY > threshold);
					}
					else
					{
						// TODO: This controller is not plugged in.
					}
				}

				// Sound output test
				SDL_LockAudio();
				int byteToLock = (soundOutput.runningSampleIndex *
								  soundOutput.bytesPerSample) %
								 soundOutput.secondaryBufferSize;
				int targetCursor = ((AudioRingBuffer.playCursor +
									 (soundOutput.latencySampleCount *
									  soundOutput.bytesPerSample)) %
									soundOutput.secondaryBufferSize);
				int bytesToWrite;
				if (byteToLock > targetCursor)
				{
					bytesToWrite =
						(soundOutput.secondaryBufferSize - byteToLock);
					bytesToWrite += targetCursor;
				}
				else
				{
					bytesToWrite = targetCursor - byteToLock;
				}

				SDL_UnlockAudio();

				GameSoundOutputBuffer soundBuffer = {};
				soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
				soundBuffer.sampleCount =
					bytesToWrite / soundOutput.bytesPerSample;
				soundBuffer.samples = samples;

				GameOffscreenBuffer buffer = {};
				buffer.memory = GlobalBackbuffer.memory;
				buffer.width = GlobalBackbuffer.width;
				buffer.height = GlobalBackbuffer.height;
				buffer.pitch = GlobalBackbuffer.pitch;
				GameUpdateAndRender(&gameMemory, newInput, &buffer,
									&soundBuffer);

				GameInput *temp = newInput;
				newInput = oldInput;
				oldInput = temp;

				SDLFillSoundBuffer(&soundOutput, byteToLock, bytesToWrite,
								   &soundBuffer);

				if (SDLGetSecondsElapsed(lastCounter,
										 SDL_GetPerformanceCounter()) <
					targetSecondsPerFrame)
				{
					int32 TimeToSleep =
						((targetSecondsPerFrame -
						  SDLGetSecondsElapsed(lastCounter,
											   SDL_GetPerformanceCounter())) *
						 1000) -
						1;
					if (TimeToSleep > 0)
					{
						SDL_Delay(TimeToSleep);
					}
					assert(
						SDLGetSecondsElapsed(lastCounter,
											 SDL_GetPerformanceCounter()) <
						targetSecondsPerFrame) while (SDLGetSecondsElapsed(lastCounter,
																		   SDL_GetPerformanceCounter()) <
													  targetSecondsPerFrame)
					{
						// Waiting...
					}
				}

				// Get this before SDLUpdateWindow() so that we don't keep
				// missing VBlanks.
				uint64 endCounter = SDL_GetPerformanceCounter();

				SDLUpdateWindow(window, renderer, &GlobalBackbuffer);
				uint64 endCycleCount = _rdtsc();
				uint64 cyclesElapsed = endCycleCount - lastCycleCount;
				uint64 counterElapsed = endCounter - lastCounter;

				real64 msPerFrame = (((1000.0f * (real64)counterElapsed) /
									  (real64)perfCountFrequency));
				real64 fps =
					(real64)perfCountFrequency / (real64)counterElapsed;
				real64 mcpf = ((real64)cyclesElapsed / (1000.0f * 1000.0f));

				printf("%.02fms/f, %.02f/s, %.02fmc/f\n", msPerFrame, fps,
					   mcpf);

				lastCycleCount = endCycleCount;
				lastCounter = endCounter;
			}
		}
		else
		{
			// TODO(casey): Logging
		}
	}
	else
	{
		// TODO(casey): Logging
	}

	SDLCloseGameControllers();
	SDL_Quit();
	return (0);
}
