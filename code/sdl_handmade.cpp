#include <SDL.h>
#include <SDL_audio.h>
#include <cstdlib>
#include <cstring>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

// TODO(bruno): estudar mmap

// NOTE: MAP_ANONYMOUS is not defined on Mac OS X and some other UNIX systems.
// On the vast majority of those systems, one can use MAP_ANON instead.
// Huge thanks to Adam Rosenfield for investigating this, and suggesting this
// workaround:
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct Backbuffer
{
	// NOTE(bruno): Pixels are always 32-bits wide, Memory Order BB GG RR XX
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

global_variable Backbuffer globalBackbuffer;
global_variable AudioRingBuffer globalAudioRingBuffer;

#define MAX_CONTROLLERS 4
SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

WindowDimension GetWindowDimension(SDL_Window *window)
{
	WindowDimension result;

	SDL_GetWindowSize(window, &result.width, &result.height);

	return (result);
}

internal void RenderWeirdGradient(Backbuffer Buffer, int BlueOffset,
								  int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer.memory;
	for (int Y = 0; Y < Buffer.height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer.width; ++X)
		{
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer.pitch;
	}
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
							  Backbuffer Buffer)
{
	SDL_UpdateTexture(Buffer.texture, 0, Buffer.memory, Buffer.pitch);

	SDL_RenderCopy(Renderer, Buffer.texture, 0, 0);

	SDL_RenderPresent(Renderer);
}

bool HandleEvent(SDL_Event *event)
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

		case SDL_WINDOWEVENT:
		{
			switch (event->window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					SDL_Window *window =
						SDL_GetWindowFromID(event->window.windowID);
					SDL_Renderer *renderer = SDL_GetRenderer(window);
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
					SDLUpdateWindow(window, renderer, globalBackbuffer);
				}
				break;
			}
		}
		break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			SDL_Keycode keycode = event->key.keysym.sym;
			bool wasDown = false;
			if (event->key.state == SDL_RELEASED)
				wasDown = true;
			else if (event->key.repeat != 0)
				wasDown = true;
			if (keycode == SDLK_w)
				printf("w\n");
		};
		break;
	}

	return (shouldQuit);
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
	audioSettings.samples = 1024;
	audioSettings.callback = &SDLAudioCallback;
	audioSettings.userdata = &globalAudioRingBuffer;

	globalAudioRingBuffer.size = bufferSize;
	globalAudioRingBuffer.data = malloc(bufferSize);
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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC |
			 SDL_INIT_AUDIO);

	SDL_Window *window = SDL_CreateWindow(
		"Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640,
		480, SDL_WINDOW_RESIZABLE);
	if (window)
	{
		InitializeControllers();

		SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer)
		{
			bool running = true;
			WindowDimension dimension = GetWindowDimension(window);
			SDLResizeTexture(&globalBackbuffer, renderer, dimension.width,
							 dimension.height);
			int xOffset = 0;
			int yOffset = 0;

			int sampleRate = 48000;
			int toneHz = 256;
			int16 toneVolume = 3000;
			uint32 runningSampleIndex = 0;
			int squareWavePeriod = sampleRate / toneHz;
			int halfSquareWavePeriod = squareWavePeriod / 2;
			int bytesPerSample = sizeof(int16) * 2;
			int secondaryBufferSize = sampleRate * bytesPerSample;
			// TODO(bruno): samplespersecond/60 because we want to write 800
			// frame. but this calculation probably shouldn't be here.

			// int targetQueueBytes = sampleRate * bytesPerSample;
			// int bytesToWrite = targetQueueBytes - SDL_GetQueuedAudioSize(1);

			InitializeAudio(sampleRate, secondaryBufferSize);
			bool soundPlaying = false;

			while (running)
			{
				SDL_Event event;
				while (SDL_PollEvent(&event))
				{
					if (HandleEvent(&event))
					{
						running = false;
					}
				}

				for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
					 controllerIndex++)
				{
					SDL_GameController *controller =
						ControllerHandles[controllerIndex];

					if (controller != NULL &&
						SDL_GameControllerGetAttached(controller))
					{
						bool up = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
						bool down = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
						bool left = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
						bool right = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
						bool start = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_START);
						bool back = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_BACK);
						bool leftShoulder = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
						bool rightShoulder = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
						bool aButton = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_A);
						bool bButton = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_B);
						bool xButton = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_X);
						bool yButton = SDL_GameControllerGetButton(
							controller, SDL_CONTROLLER_BUTTON_Y);

						int16 stickX = SDL_GameControllerGetAxis(
							controller, SDL_CONTROLLER_AXIS_LEFTX);
						int16 stickY = SDL_GameControllerGetAxis(
							controller, SDL_CONTROLLER_AXIS_LEFTY);

						if (aButton)
						{
							yOffset += 2;
						}
						SDL_Haptic *haptic = RumbleHandles[controllerIndex];
						if (bButton && haptic)
						{
							SDL_HapticRumblePlay(haptic, 0.5f, 2000);
						}
					}
				}

				RenderWeirdGradient(globalBackbuffer, xOffset, yOffset);

				SDL_LockAudio();
				// TODO(bruno): what in the actual skibidi whippy flying fuck
				// is happening here
				int byteToLock =
					runningSampleIndex * bytesPerSample % secondaryBufferSize;
				int bytesToWrite;
				if (byteToLock == globalAudioRingBuffer.playCursor)
				{
					bytesToWrite = secondaryBufferSize;
				}
				else if (byteToLock > globalAudioRingBuffer.playCursor)
				{
					bytesToWrite = (secondaryBufferSize - byteToLock);
					bytesToWrite += globalAudioRingBuffer.playCursor;
				}
				else
				{
					bytesToWrite =
						globalAudioRingBuffer.playCursor - byteToLock;
				}

				void *region1 =
					(uint8 *)globalAudioRingBuffer.data + byteToLock;
				int region1Size = bytesToWrite;
				if (region1Size + byteToLock > secondaryBufferSize)
					region1Size = secondaryBufferSize - byteToLock;
				void *Region2 = globalAudioRingBuffer.data;
				int Region2Size = bytesToWrite - region1Size;
				SDL_UnlockAudio();
				int Region1SampleCount = region1Size / bytesPerSample;
				int16 *SampleOut = (int16 *)region1;
				for (int SampleIndex = 0; SampleIndex < Region1SampleCount;
					 ++SampleIndex)
				{
					int16 SampleValue =
						((runningSampleIndex++ / halfSquareWavePeriod) % 2)
							? toneVolume
							: -toneVolume;
					*SampleOut++ = SampleValue;
					*SampleOut++ = SampleValue;
				}

				int Region2SampleCount = Region2Size / bytesPerSample;
				SampleOut = (int16 *)Region2;
				for (int SampleIndex = 0; SampleIndex < Region2SampleCount;
					 ++SampleIndex)
				{
					int16 SampleValue =
						((runningSampleIndex++ / halfSquareWavePeriod) % 2)
							? toneVolume
							: -toneVolume;
					*SampleOut++ = SampleValue;
					*SampleOut++ = SampleValue;
				}

				if (!soundPlaying)
				{
					SDL_PauseAudio(0);
					soundPlaying = true;
				}

				SDLUpdateWindow(window, renderer, globalBackbuffer);

				++xOffset;
			}
		}
		else
		{
			// TODO(bruno): Logging
		}
	}
	else
	{
		// TODO(bruno): Logging
	}

	// TODO(bruno): maybe we should close the controllers and rumble handles on
	// exit. or maybe not, since the OS should take care of that when the
	// process ends.
	// TODO(bruno): same for audio.
	SDL_Quit();
	return (0);
}
