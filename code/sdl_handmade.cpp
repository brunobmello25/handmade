#include "SDL_events.h"
#include "SDL_keycode.h"
#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

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

struct WindowDimension
{
	int width;
	int height;
};

global_variable Backbuffer globalBackbuffer;

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

// TODO(bruno): maybe we should close the controllers and rumble handles on
// exit. or maybe not, since the OS should take care of that when the process
// ends.
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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);

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

	SDL_Quit();
	return (0);
}
