#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

#include "handmade.cpp"

global_variable bool Running = true;

global_variable int XOffset = 0;
global_variable int YOffset = 0;

struct WindowDimension
{
	int width;
	int height;
};

WindowDimension GetWindowDimension(SDL_Window *Window)
{
	WindowDimension Result;
	SDL_GetWindowSize(Window, &Result.width, &Result.height);
	return Result;
}

struct BackBuffer
{
	SDL_Texture *texture;
	void *memory;
	int width;
	int height;
	int bytesPerPixel;
};

global_variable BackBuffer globalBackbuffer;

internal void RenderWeirdGradient(BackBuffer buffer, int xOffset, int yOffset)
{
	int pitch = buffer.width * buffer.bytesPerPixel;
	uint8_t *row = (uint8_t *)buffer.memory;
	for (int Y = 0; Y < buffer.height; ++Y)
	{
		uint32_t *Pixel = (uint32_t *)row;
		for (int X = 0; X < buffer.width; ++X)
		{
			uint8_t Blue = (X + xOffset);
			uint8_t Green = (Y + yOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		row += pitch;
	}
}

void SDLResizeTexture(BackBuffer *buffer, SDL_Renderer *renderer, int width,
					  int height, int xoffset, int yoffset)
{
	if (buffer->memory)
	{
		free(buffer->memory);
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
	buffer->bytesPerPixel = 4;

	buffer->memory =
		malloc(buffer->width * buffer->height * buffer->bytesPerPixel);

	RenderWeirdGradient(*buffer, xoffset, yoffset);
}

void SDLUpdateWindow(BackBuffer buffer, SDL_Window *window,
					 SDL_Renderer *renderer)
{
	// TODO(bruno): Handle error
	int pitch = buffer.width * buffer.bytesPerPixel;
	SDL_UpdateTexture(buffer.texture, NULL, buffer.memory, pitch);

	SDL_RenderCopy(renderer, buffer.texture, NULL, NULL);

	SDL_RenderPresent(renderer);
}

bool HandleEvent(SDL_Event *event)
{
	bool shouldQuit = false;

	switch (event->type)
	{
		case SDL_QUIT:
		{
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

					SDLUpdateWindow(globalBackbuffer, window, renderer);
				}
				break;
			}
		}
		break;
	}

	return shouldQuit;
}

int main(int argc, char *argv[])
{

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *Window = SDL_CreateWindow(
		"Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280,
		720, SDL_WINDOW_RESIZABLE);

	if (Window)
	{
		SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

		if (Renderer)
		{
			bool running = true;
			int Width, Height;
			WindowDimension Dimension = GetWindowDimension(Window);
			SDLResizeTexture(&globalBackbuffer, Renderer, Dimension.width,
							 Dimension.height, XOffset, YOffset);

			while (running)
			{
				SDL_Event Event;
				while (SDL_PollEvent(&Event))
				{
					if (HandleEvent(&Event))
					{
						running = false;
					}
				}
				RenderWeirdGradient(globalBackbuffer, XOffset, YOffset);
				SDLUpdateWindow(globalBackbuffer, Window, Renderer);

				++XOffset;
			}
		}
		else
		{
			// TODO:(Bruno): Logging
		}
	}
	else
	{
		// TODO:(Bruno): Logging
	}

	return (0);
}
