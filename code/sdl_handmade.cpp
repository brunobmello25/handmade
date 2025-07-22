#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

global_variable bool Running = true;

struct sdl_offscreen_buffer
{
	SDL_Texture *Texture;
	void *Memory;
	int Width;
	int Height;
	int BytesPerPixel;
};

global_variable sdl_offscreen_buffer GlobalBackbuffer;

internal void RenderWeirdGradient(sdl_offscreen_buffer Buffer, int XOffset,
								  int YOffset)
{
	int Width = Buffer.Width;
	int Height = Buffer.Height;

	int Pitch = Width * Buffer.BytesPerPixel;
	uint8_t *Row = (uint8_t *)Buffer.Memory;
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = 0; X < Buffer.Width; ++X)
		{
			uint8_t Blue = (X + XOffset);
			uint8_t Green = (Y + YOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Pitch;
	}
}

void SDLResizeTexture(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer,
					  int Width, int Height)
{
	if (Buffer->Memory)
	{
		free(Buffer->Memory);
	}

	if (Buffer->Texture)
	{
		SDL_DestroyTexture(Buffer->Texture);
	}

	Buffer->Texture =
		SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, Width, Height);
	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->Memory =
		malloc(Buffer->Width * Buffer->Height * Buffer->BytesPerPixel);

	RenderWeirdGradient(*Buffer, 128, 0);
}

void SDLUpdateWindow(sdl_offscreen_buffer Buffer, SDL_Window *Window,
					 SDL_Renderer *Renderer)
{

	// TODO: (Bruno) Handle error
	SDL_UpdateTexture(Buffer.Texture, NULL, Buffer.Memory,
					  Buffer.Width * Buffer.Height);

	SDL_RenderCopy(Renderer, Buffer.Texture, NULL, NULL);

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

					SDLResizeTexture(&GlobalBackbuffer, Renderer,
									 Event->window.data1, Event->window.data2);
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

					SDLUpdateWindow(GlobalBackbuffer, Window, Renderer);
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
			SDLResizeTexture(&GlobalBackbuffer, Renderer, Width, Height);

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
				RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);
				SDLUpdateWindow(GlobalBackbuffer, Window, Renderer);

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
