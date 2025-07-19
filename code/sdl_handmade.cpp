#include "SDL_events.h"
#include "SDL_video.h"
#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

global_variable bool Running = true;

global_variable SDL_Texture *Texture;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderWeirdGradient(int XOffset, int YOffset)
{
	int Width = BitmapWidth;
	int Height = BitmapHeight;

	int Pitch = Width * BytesPerPixel;
	uint8_t *Row = (uint8_t *)BitmapMemory;
	for (int Y = 0; Y < BitmapHeight; ++Y)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = 0; X < BitmapWidth; ++X)
		{
			uint8_t Blue = (X + XOffset);
			uint8_t Green = (Y + YOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Pitch;
	}
}

void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height)
{
	if (BitmapMemory)
	{
		free(BitmapMemory);
	}

	if (Texture)
	{
		SDL_DestroyTexture(Texture);
	}

	Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888,
								SDL_TEXTUREACCESS_STREAMING, Width, Height);
	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapMemory = malloc(BitmapWidth * BitmapHeight * BytesPerPixel);

	RenderWeirdGradient(128, 0);
}

void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer)
{

	// TODO: (Bruno) Handle error
	SDL_UpdateTexture(Texture, NULL, BitmapMemory, BitmapWidth * BytesPerPixel);

	SDL_RenderCopy(Renderer, Texture, NULL, NULL);

	SDL_RenderPresent(Renderer);
}

bool HandleEvent(SDL_Event *Event)
{
	bool ShouldQuit = false;

	switch (Event->type)
	{
		case SDL_QUIT:
		{
			printf("SDL_QUIT\n");
			ShouldQuit = true;
		}
		break;

		case SDL_WINDOWEVENT:
		{
			switch (Event->window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					SDL_Window *Window =
						SDL_GetWindowFromID(Event->window.windowID);
					SDL_Renderer *Renderer = SDL_GetRenderer(Window);

					printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n",
						   Event->window.data1, Event->window.data2);

					SDLResizeTexture(Renderer, Event->window.data1,
									 Event->window.data2);
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
						SDL_GetWindowFromID(Event->window.windowID);
					SDL_Renderer *Renderer = SDL_GetRenderer(Window);

					SDLUpdateWindow(Window, Renderer);
				}
				break;
			}
		}
		break;
	}

	return ShouldQuit;
}

int main(int argc, char *argv[])
{

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *Window = SDL_CreateWindow(
		"Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640,
		480, SDL_WINDOW_RESIZABLE);

	if (Window)
	{
		SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

		if (Renderer)
		{
			bool Running = true;
			int Width, Height;
			SDL_GetWindowSize(Window, &Width, &Height);
			SDLResizeTexture(Renderer, Width, Height);

			int XOffset = 0;
			int YOffset = 0;
			while (Running)
			{
				SDL_Event Event;
				while (SDL_PollEvent(&Event))
				{
					if (HandleEvent(&Event))
					{
						Running = false;
					}
				}
				RenderWeirdGradient(XOffset, YOffset);
				SDLUpdateWindow(Window, Renderer);

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
