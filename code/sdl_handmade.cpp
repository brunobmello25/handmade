#include "SDL_render.h"
#include <SDL.h>
#include <cstdio>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

global_variable bool Running = true;

global_variable SDL_Texture *Texture;
global_variable void *Pixels;
global_variable int TextureWidth;

void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height)
{
	if (Pixels)
	{
		free(Pixels);
	}

	if (Texture)
	{
		SDL_DestroyTexture(Texture);
	}

	Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888,
								SDL_TEXTUREACCESS_STREAMING, Width, Height);
	TextureWidth = Width;

	Pixels = malloc(Width * Height * 4);
}

void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer)
{
	// TODO: (Bruno) Handle error
	SDL_UpdateTexture(Texture, NULL, Pixels, TextureWidth * 4);

	SDL_RenderCopy(Renderer, Texture, NULL, NULL);

	SDL_RenderPresent(Renderer);
}

void HandleEvent(SDL_Event *Event)
{

	switch (Event->type)
	{
		case SDL_QUIT:
		{
			printf("SDL_QUIT\n");
			Running = false;
		}
		break;
		case SDL_WINDOWEVENT:
		{
			switch (Event->window.event)
			{
				case SDL_WINDOWEVENT_RESIZED:
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
			while (Running)
			{
				SDL_Event event;
				SDL_WaitEvent(&event);

				HandleEvent(&event);
			}
		}
	}

	return (0);
}
