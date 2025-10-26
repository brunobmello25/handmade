// vim: set ts=4 sw=4 expandtab:
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

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

struct SDLBackBuffer
{
	// NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
	SDL_Texture *texture;
	void *memory;
	int width;
	int height;
	int pitch;
};

struct SDLWindowDimension
{
	int width;
	int height;
};

global_variable SDLBackBuffer globalBackbuffer;

#define MAX_CONTROLLERS 4
SDL_GameController *controllerHandles[MAX_CONTROLLERS];
SDL_Haptic *rumbleHandles[MAX_CONTROLLERS];

SDLWindowDimension SDLGetWindowDimension(SDL_Window *window)
{
	SDLWindowDimension result;

	SDL_GetWindowSize(window, &result.width, &result.height);

	return (result);
}

internal void RenderWeirdGradient(SDLBackBuffer *buffer, int blueOffset,
								  int greenOffset)
{
	uint8 *row = (uint8 *)buffer->memory;
	for (int y = 0; y < buffer->height; ++y)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x < buffer->width; ++x)
		{
			uint8 blue = (x + blueOffset);
			uint8 green = (y + greenOffset);

			*pixel++ = ((green << 8) | blue);
		}

		row += buffer->pitch;
	}
}

internal void SDLResizeTexture(SDLBackBuffer *buffer, SDL_Renderer *renderer,
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

internal void SDLUpdateWindow(SDL_Window *window, SDL_Renderer *renderer,
							  SDLBackBuffer *buffer)
{
	SDL_UpdateTexture(buffer->texture, 0, buffer->memory, buffer->pitch);

	SDL_RenderCopy(renderer, buffer->texture, 0, 0);

	SDL_RenderPresent(renderer);
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

		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			SDL_Keycode keycode = event->key.keysym.sym;
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
				if (keycode == SDLK_w)
				{
				}
				else if (keycode == SDLK_a)
				{
				}
				else if (keycode == SDLK_s)
				{
				}
				else if (keycode == SDLK_d)
				{
				}
				else if (keycode == SDLK_q)
				{
				}
				else if (keycode == SDLK_e)
				{
				}
				else if (keycode == SDLK_UP)
				{
				}
				else if (keycode == SDLK_LEFT)
				{
				}
				else if (keycode == SDLK_DOWN)
				{
				}
				else if (keycode == SDLK_RIGHT)
				{
				}
				else if (keycode == SDLK_ESCAPE)
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
				else if (keycode == SDLK_SPACE)
				{
				}
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
					SDLUpdateWindow(window, renderer, &globalBackbuffer);
				}
				break;
			}
		}
		break;
	}

	return (shouldQuit);
}

internal void SDLOpenGameControllers()
{
	int maxJoysticks = SDL_NumJoysticks();
	int controllerIndex = 0;
	for (int JoystickIndex = 0; JoystickIndex < maxJoysticks; ++JoystickIndex)
	{
		if (!SDL_IsGameController(JoystickIndex))
		{
			continue;
		}
		if (controllerIndex >= MAX_CONTROLLERS)
		{
			break;
		}
		controllerHandles[controllerIndex] =
			SDL_GameControllerOpen(JoystickIndex);
		rumbleHandles[controllerIndex] = SDL_HapticOpen(JoystickIndex);
		if (rumbleHandles[controllerIndex] &&
			SDL_HapticRumbleInit(rumbleHandles[controllerIndex]) != 0)
		{
			SDL_HapticClose(rumbleHandles[controllerIndex]);
			rumbleHandles[controllerIndex] = 0;
		}

		controllerIndex++;
	}
}

internal void SDLCloseGameControllers()
{
	for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS;
		 ++ControllerIndex)
	{
		if (controllerHandles[ControllerIndex])
		{
			if (rumbleHandles[ControllerIndex])
				SDL_HapticClose(rumbleHandles[ControllerIndex]);
			SDL_GameControllerClose(controllerHandles[ControllerIndex]);
		}
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
	// Initialise our Game Controllers:
	SDLOpenGameControllers();
	// Create our window.
	SDL_Window *window = SDL_CreateWindow(
		"Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640,
		480, SDL_WINDOW_RESIZABLE);
	if (window)
	{
		// Create a "Renderer" for our window.
		SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer)
		{
			bool running = true;
			SDLWindowDimension dimension = SDLGetWindowDimension(window);
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

				// Poll our controllers for input.
				for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
					 ++controllerIndex)
				{
					if (controllerHandles[controllerIndex] != 0 &&
						SDL_GameControllerGetAttached(
							controllerHandles[controllerIndex]))
					{
						// NOTE: We have a controller with index
						// ControllerIndex.
						bool Up = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_UP);
						bool Down = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_DOWN);
						bool Left = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_LEFT);
						bool Right = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
						bool Start = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_START);
						bool Back = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_BACK);
						bool LeftShoulder = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
						bool RightShoulder = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
						bool aButton = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_A);
						bool bButton = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_B);
						bool XButton = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_X);
						bool YButton = SDL_GameControllerGetButton(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_BUTTON_Y);

						int16 StickX = SDL_GameControllerGetAxis(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_AXIS_LEFTX);
						int16 StickY = SDL_GameControllerGetAxis(
							controllerHandles[controllerIndex],
							SDL_CONTROLLER_AXIS_LEFTY);

						if (aButton)
						{
							yOffset += 2;
						}
						if (bButton)
						{
							if (rumbleHandles[controllerIndex])
							{
								SDL_HapticRumblePlay(
									rumbleHandles[controllerIndex], 0.5f, 2000);
							}
						}
					}
					else
					{
						// TODO: This controller is not plugged in.
					}
				}

				RenderWeirdGradient(&globalBackbuffer, xOffset, yOffset);
				SDLUpdateWindow(window, renderer, &globalBackbuffer);

				++xOffset;
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
