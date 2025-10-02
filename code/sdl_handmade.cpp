#include <SDL.h>
#include <SDL_audio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// TODO(bruno): estudar mmap

// NOTE: MAP_ANONYMOUS is not defined on Mac OS X and some other UNIX systems.
// On the vast majority of those systems, one can use MAP_ANON instead.
// Huge thanks to Adam Rosenfield for investigating this, and suggesting this
// workaround:
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#include "handmade.cpp"

struct Backbuffer
{
	// NOTE(casey): Pixels are always 32-bits wide, Memory Order BB GG RR XX
	SDL_Texture *texture;
	void *memory;
	int width;
	int height;
	int pitch;
};

struct AudioRingBuffer
{
	int size;
	int writeCursor;
	int playCursor;
	void *data;
};

struct WindowDimension
{
	int width;
	int height;
};

struct SoundOutput
{
	int sampleRate;
	uint32 runningSampleIndex;
	int bytesPerSample;
	int secondaryBufferSize;
	int latencySampleCount;
	float tSine;
};

global_variable Backbuffer globalBackbuffer;
global_variable AudioRingBuffer globalAudioRingBuffer;
global_variable bool globalRunning;

#define MAX_CONTROLLERS 4
SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

WindowDimension GetWindowDimension(SDL_Window *window)
{
	WindowDimension result;

	SDL_GetWindowSize(window, &result.width, &result.height);

	return (result);
}

inline uint32 SafeTruncateUInt64(uint64 Value)
{
	assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return (Result);
}

internal DEBUGReadFileResult DEBUGPlatformReadEntireFile(const char *filename)
{
	DEBUGReadFileResult result = {};

	int handle = open(filename, O_RDONLY);
	if (handle == -1)
	{
		return result;
	}

	struct stat fileStat;
	if (fstat(handle, &fileStat) == -1)
	{
		close(handle);
		return result;
	}
	result.contentsSize = SafeTruncateUInt64(fileStat.st_size);
	result.contents = malloc(result.contentsSize);
	if (result.contents)
	{
		result.contentsSize = 0;
		close(handle);
		return result;
	}

	uint32 bytesToRead = result.contentsSize;
	uint8 *at = (uint8 *)result.contents;
	while (bytesToRead)
	{
		ssize_t bytesRead = read(handle, at, bytesToRead);
		if (bytesRead == -1)
		{
			free(result.contents);
			result.contents = 0;
			result.contentsSize = 0;
			close(handle);
			return result;
		}
		bytesToRead -= bytesRead;
		at += bytesRead;
	}

	close(handle);
	return result;
}

internal void DEBUGPlatformFreeFileMemory(void *memory) { free(memory); }

internal bool DEBUGPlatformWriteEntireFile(char *filename, void *memory,
										   uint32 size)
{
	int handle = open(filename, O_WRONLY | O_CREAT,
					  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (handle == -1)
	{
		return false;
	}
	uint32 bytesToWrite = size;
	uint8 *at = (uint8 *)memory;
	while (bytesToWrite)
	{
		ssize_t bytesWritten = write(handle, at, bytesToWrite);
		if (bytesWritten == -1)
		{
			close(handle);
			return false;
		}
		bytesToWrite -= bytesWritten;
		at += bytesWritten;
	}
	close(handle);
	return true;
}

internal void SDLResizeTexture(Backbuffer *buffer, SDL_Renderer *renderer,
							   int width, int height)
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

internal void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer,
							  Backbuffer *Buffer)
{
	SDL_UpdateTexture(Buffer->texture, 0, Buffer->memory, Buffer->pitch);

	SDL_RenderCopy(Renderer, Buffer->texture, 0, 0);

	SDL_RenderPresent(Renderer);
}

internal void SDLProcessKeyPress(GameButtonState *newState, bool isDown)
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

			// NOTE: In the windows version, we used "if (IsDown != WasDown)"
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
						printf("IsDown ");
					}
					if (wasDown)
					{
						printf("WasDown");
					}
					printf("\n");
				}
				else if (keyCode == SDLK_SPACE)
				{
				}
			}

			bool AltKeyWasDown = (event->key.keysym.mod & KMOD_ALT);
			if (keyCode == SDLK_F4 && AltKeyWasDown)
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
					SDL_Window *Window =
						SDL_GetWindowFromID(event->window.windowID);
					SDL_Renderer *Renderer = SDL_GetRenderer(Window);
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
					SDL_Window *Window =
						SDL_GetWindowFromID(event->window.windowID);
					SDL_Renderer *Renderer = SDL_GetRenderer(Window);
					SDLUpdateWindow(Window, Renderer, &globalBackbuffer);
				}
				break;
			}
		}
		break;
	}

	return shouldQuit;
}

void InitializeControllers()
{

	int maxJoysticks = SDL_NumJoysticks();
	int controllerIndex = 0;
	for (int joystickIndex = 0; joystickIndex < maxJoysticks; joystickIndex++)
	{
		if (!SDL_IsGameController(joystickIndex))
			continue;

		if (controllerIndex >= MAX_CONTROLLERS)
			break;

		SDL_GameController *controller = SDL_GameControllerOpen(joystickIndex);
		ControllerHandles[controllerIndex] = controller;

		SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
		SDL_Haptic *haptic = SDL_HapticOpenFromJoystick(joystick);
		RumbleHandles[controllerIndex] = haptic;
		if (haptic && SDL_HapticRumbleInit(haptic) != 0)
		{
			SDL_HapticClose(haptic);
			RumbleHandles[controllerIndex] = NULL;
		}

		controllerIndex++;
	}
}

internal void SDLAudioCallback(void *userdata, Uint8 *AudioData, int length)
{
	AudioRingBuffer *ringbuffer = (AudioRingBuffer *)userdata;

	int region1size = length;
	int region2size = 0;
	if (ringbuffer->playCursor + length > ringbuffer->size)
	{
		region1size = ringbuffer->size - ringbuffer->playCursor;
		region2size = length - region1size;
	}

	memcpy(AudioData, (uint8 *)(ringbuffer->data) + ringbuffer->playCursor,
		   region1size);
	memcpy(&AudioData[region1size], ringbuffer->data, region2size);
	ringbuffer->playCursor =
		(ringbuffer->playCursor + length) % ringbuffer->size;
	ringbuffer->writeCursor =
		(ringbuffer->playCursor + 2048) % ringbuffer->size;
}

internal void InitializeAudio(int32 sampleRate, int32 bufferSize)
{

	SDL_AudioSpec audioSettings = {0};
	// TODO(bruno): maybe extract these to globals?
	audioSettings.freq = sampleRate;
	audioSettings.format = AUDIO_S16LSB;
	audioSettings.channels = 2;
	audioSettings.samples = 512;
	audioSettings.callback = &SDLAudioCallback;
	audioSettings.userdata = &globalAudioRingBuffer;

	globalAudioRingBuffer.size = bufferSize;
	globalAudioRingBuffer.data = calloc(bufferSize, 1);
	globalAudioRingBuffer.playCursor = globalAudioRingBuffer.writeCursor = 0;

	SDL_OpenAudio(&audioSettings, NULL);

	printf("Initialised an Audio device at frequency %d Hz, %d Channels, "
		   "buffer size %d\n",
		   audioSettings.freq, audioSettings.channels, audioSettings.samples);

	if (audioSettings.format != AUDIO_S16LSB)
	{
		printf("Didn't get AUDIO_S16LSB as sample format!\n");
		SDL_CloseAudio();
	}
}

internal void SDLFillSoundBuffer(SoundOutput *soundOutput, int byteToLock,
								 int bytesToWrite,
								 GameSoundBuffer *gameSoundBuffer)
{
	int16 *samples = gameSoundBuffer->samples;

	void *region1 = (uint8 *)globalAudioRingBuffer.data + byteToLock;
	int region1Size = bytesToWrite;
	if (region1Size + byteToLock > soundOutput->secondaryBufferSize)
	{
		region1Size = soundOutput->secondaryBufferSize - byteToLock;
	}
	void *region2 = globalAudioRingBuffer.data;
	int region2Size = bytesToWrite - region1Size;
	int region1SampleCount = region1Size / soundOutput->bytesPerSample;
	int16 *SampleOut = (int16 *)region1;
	for (int SampleIndex = 0; SampleIndex < region1SampleCount; ++SampleIndex)
	{
		*SampleOut++ = *samples++;
		*SampleOut++ = *samples++;

		++soundOutput->runningSampleIndex;
	}

	int region2SampleCount = region2Size / soundOutput->bytesPerSample;
	SampleOut = (int16 *)region2;
	for (int SampleIndex = 0; SampleIndex < region2SampleCount; ++SampleIndex)
	{
		*SampleOut++ = *samples++;
		*SampleOut++ = *samples++;

		++soundOutput->runningSampleIndex;
	}
}

internal float SDLProcessGameControllerAxisValue(int16 Value,
												 int16 DeadZoneThreshold)
{
	float Result = 0;

	if (Value < -DeadZoneThreshold)
	{
		Result = (float)((Value + DeadZoneThreshold) /
						 (32768.0f - DeadZoneThreshold));
	}
	else if (Value > DeadZoneThreshold)
	{
		Result = (float)((Value - DeadZoneThreshold) /
						 (32767.0f - DeadZoneThreshold));
	}

	return (Result);
}

internal void SDLProcessControllerButton(GameButtonState *oldState,
										 GameButtonState *newState, bool isDown)
{
	newState->endedDown = isDown;
	newState->halfTransitionCount +=
		((newState->endedDown == oldState->endedDown) ? 0 : 1);
}

internal void SDLCloseGameControllers()
{
	for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
		 controllerIndex++)
	{
		if (ControllerHandles[controllerIndex])
		{
			if (RumbleHandles[controllerIndex])
				SDL_HapticClose(RumbleHandles[controllerIndex]);
			SDL_GameControllerClose(ControllerHandles[controllerIndex]);
		}
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC |
			 SDL_INIT_AUDIO);

	uint64 performanceFrequency = SDL_GetPerformanceFrequency();

	InitializeControllers();

	SDL_Window *window = SDL_CreateWindow(
		"Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640,
		480, SDL_WINDOW_RESIZABLE);
	if (window)
	{
		SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

		if (renderer)
		{
			globalRunning = true;
			WindowDimension dimension = GetWindowDimension(window);
			SDLResizeTexture(&globalBackbuffer, renderer, dimension.width,
							 dimension.height);

			GameInput inputs[2] = {};
			GameInput *newInput = &inputs[0];
			GameInput *oldInput = &inputs[1];

			SoundOutput soundOutput = {};
			soundOutput.sampleRate = 48000;
			soundOutput.runningSampleIndex = 0;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.secondaryBufferSize =
				soundOutput.sampleRate * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.sampleRate / 15;
			soundOutput.tSine = 0.0f;

			InitializeAudio(soundOutput.sampleRate,
							soundOutput.secondaryBufferSize);
			int16 *samples = (int16 *)calloc(soundOutput.sampleRate,
											 soundOutput.bytesPerSample);
			SDL_PauseAudio(0);

#if HANDMADE_INTERNAL
			void *baseAddress = (void *)Terabytes((uint64)2);
#else
			void *baseAddress = (void *)0;
#endif
			GameMemory gameMemory = {};
			gameMemory.permanentStorageSize = Megabytes(64);
			gameMemory.transientStorageSize = Gigabytes((uint64)4);

			uint64 totalStorageSize = gameMemory.transientStorageSize +
									  gameMemory.permanentStorageSize;

			// TODO(bruno): maybe touch all the pages to avoid lazy allocation.
			gameMemory.permanentStorage =
				mmap(baseAddress, totalStorageSize, PROT_READ | PROT_WRITE,
					 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

			assert(gameMemory.permanentStorage);
			gameMemory.transientStorage = (uint8 *)gameMemory.permanentStorage +
										  gameMemory.permanentStorageSize;

			uint64 lastCounter = SDL_GetPerformanceCounter();
			uint64 lastCycleCount = _rdtsc();

			while (globalRunning)
			{
				GameControllerInput *oldKeyboardController =
					GetController(oldInput, 0);
				GameControllerInput *newKeyboardController =
					GetController(newInput, 0);
				*newKeyboardController = {};
				for (int buttonIndex = 0;
					 buttonIndex < ArrayCount(newKeyboardController->buttons);
					 buttonIndex++)
				{
					newKeyboardController->buttons[buttonIndex].endedDown =
						oldKeyboardController->buttons[buttonIndex].endedDown;
				}

				SDL_Event event;
				while (SDL_PollEvent(&event))
				{
					if (HandleEvent(&event, newKeyboardController))
					{
						globalRunning = false;
					}
				}

				for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
					 controllerIndex++)
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

						SDLProcessControllerButton(
							&(oldController->leftShoulder),
							&(newController->leftShoulder),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_LEFTSHOULDER));

						SDLProcessControllerButton(
							&(oldController->rightShoulder),
							&(newController->rightShoulder),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

						SDLProcessControllerButton(
							&(oldController->actionDown),
							&(newController->actionDown),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_A));

						SDLProcessControllerButton(
							&(oldController->actionRight),
							&(newController->actionRight),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_B));

						SDLProcessControllerButton(
							&(oldController->actionLeft),
							&(newController->actionLeft),
							SDL_GameControllerGetButton(
								ControllerHandles[controllerIndex],
								SDL_CONTROLLER_BUTTON_X));

						SDLProcessControllerButton(
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

						float Threshold = 0.5f;
						SDLProcessControllerButton(
							&(oldController->moveLeft),
							&(newController->moveLeft),
							newController->stickAverageX < -Threshold);
						SDLProcessControllerButton(
							&(oldController->moveRight),
							&(newController->moveRight),
							newController->stickAverageX > Threshold);
						SDLProcessControllerButton(
							&(oldController->moveUp), &(newController->moveUp),
							newController->stickAverageY < -Threshold);
						SDLProcessControllerButton(
							&(oldController->moveDown),
							&(newController->moveDown),
							newController->stickAverageY > Threshold);
					}
					else
					{
						// TODO(bruno): controller not plugged in
					}
				}

				// TODO(bruno): what in the actual skibidi whippy flying
				// fuck is happening here
				SDL_LockAudio();
				int byteToLock = (soundOutput.runningSampleIndex *
								  soundOutput.bytesPerSample) %
								 soundOutput.secondaryBufferSize;
				int targetCursor = ((globalAudioRingBuffer.playCursor +
									 (soundOutput.latencySampleCount *
									  soundOutput.bytesPerSample)) %
									soundOutput.secondaryBufferSize);

				int BytesToWrite;
				if (byteToLock > targetCursor)
				{
					BytesToWrite =
						(soundOutput.secondaryBufferSize - byteToLock);
					BytesToWrite += targetCursor;
				}
				else
				{
					BytesToWrite = targetCursor - byteToLock;
				}
				SDL_UnlockAudio();

				GameSoundBuffer gameSoundBuffer = {};
				gameSoundBuffer.sampleRate = soundOutput.sampleRate;
				gameSoundBuffer.sampleCount =
					BytesToWrite / soundOutput.bytesPerSample;
				gameSoundBuffer.samples = samples;

				GameBackBuffer gameBackbuffer = {};
				gameBackbuffer.width = globalBackbuffer.width;
				gameBackbuffer.height = globalBackbuffer.height;
				gameBackbuffer.pitch = globalBackbuffer.pitch;
				gameBackbuffer.memory = globalBackbuffer.memory;

				GameUpdateAndRender(&gameMemory, newInput, &gameBackbuffer,
									&gameSoundBuffer);

				GameInput *temp = oldInput;
				oldInput = newInput;
				newInput = temp;

				SDLFillSoundBuffer(&soundOutput, byteToLock, BytesToWrite,
								   &gameSoundBuffer);

				SDLUpdateWindow(window, renderer, &globalBackbuffer);

				uint64 endCycleCount = _rdtsc();
				uint64 endCounter = SDL_GetPerformanceCounter();
				uint64 counterElapsed = endCounter - lastCounter;
				double msPerFrame = (((1000.0f * (double)counterElapsed) /
									  (double)performanceFrequency));
				double fps =
					(double)performanceFrequency / (double)counterElapsed;

				int64 cyclesElapsed = endCycleCount - lastCycleCount;
				int32 MCPF = (int32)(cyclesElapsed / (1000 * 1000));

				printf("%0.2fms/f,  %0.2ffps,  %dmc/f\n", msPerFrame, fps,
					   MCPF);
				lastCounter = endCounter;
				lastCycleCount = endCycleCount;

				// TODO(casey): should we clear this here?
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

	// TODO(bruno): maybe we should close the controllers and rumble handles on
	// exit. or maybe not, since the OS should take care of that when the
	// process ends.
	// TODO(bruno): same for audio.
	SDLCloseGameControllers();
	SDL_Quit();
	return (0);
}
