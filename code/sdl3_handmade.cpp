#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define global_variable static
#define MAX_CONTROLLERS 4

struct PlatformBackbuffer
{
	int width;
	int height;
	int pitch;
	void *memory;
	SDL_Texture *texture;
};

global_variable PlatformBackbuffer globalBackbuffer;
global_variable bool globalRunning;

global_variable SDL_Gamepad *GamepadHandles[MAX_CONTROLLERS];
global_variable int xOffset = 0, yOffset = 0;

// function that gets called everytime the window size changes, and also
// at first time, in order to allocate the backbuffer with proper dimensions
void platformResizeBackbuffer(PlatformBackbuffer *backbuffer,
							  SDL_Renderer *renderer, int width, int height)
{
	int bytesPerPixel = sizeof(int);

	if (backbuffer->memory)
		free(backbuffer->memory);

	if (backbuffer->texture)
		SDL_DestroyTexture(backbuffer->texture);

	backbuffer->width = width;
	backbuffer->height = height;
	backbuffer->pitch = width * bytesPerPixel;
	backbuffer->memory = malloc(width * height * bytesPerPixel);
	backbuffer->texture =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, width, height);
}

bool platformProcessEvents(PlatformBackbuffer *backbuffer)
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_EVENT_QUIT)
		{
			return false;
		}
		if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_ESCAPE)
		{
			return false;
		}
		if (event.type == SDL_EVENT_WINDOW_RESIZED)
		{
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

void platformUpdateWindow(PlatformBackbuffer *buffer, SDL_Window *window,
						  SDL_Renderer *renderer)
{
	SDL_UpdateTexture(buffer->texture, NULL, buffer->memory, buffer->pitch);
	SDL_RenderTexture(renderer, buffer->texture, 0, 0);
	SDL_RenderPresent(renderer);
}

void renderWeirdGradient(PlatformBackbuffer *buffer, int blueOffset,
						 int greenOffset)
{
	uint8_t *row = (uint8_t *)buffer->memory;
	for (int y = 0; y < buffer->height; ++y)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int x = 0; x < buffer->width; ++x)
		{
			uint8_t blue = (x + blueOffset);
			uint8_t green = (y + greenOffset);

			*pixel++ = (0xFF << 24) | (green << 8) | blue;
		}

		row += buffer->pitch;
	}
}

void platformLoadControllers()
{

	int gamepadCount;
	uint *ids = SDL_GetGamepads(&gamepadCount);

	for (int i = 0; i < gamepadCount && i < MAX_CONTROLLERS; i++)
	{
		SDL_Gamepad *pad = SDL_OpenGamepad(ids[i]);
		if (pad)
		{
			GamepadHandles[i] = pad;
		}
	}
}

int main(void)
{
	int initialWidth = 1920 / 2;
	int initialHeight = 1080 / 2;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
		return -1;

	platformLoadControllers();

	SDL_Window *window = SDL_CreateWindow("Handmade Hero", initialWidth,
										  initialHeight, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
	if (!window || !renderer) // TODO(bruno): proper error handling
		return -1;

	globalRunning = true;

	globalBackbuffer = {};
	platformResizeBackbuffer(&globalBackbuffer, renderer, initialWidth,
							 initialHeight);

	while (globalRunning)
	{
		globalRunning = platformProcessEvents(&globalBackbuffer);

		renderWeirdGradient(&globalBackbuffer, xOffset, yOffset);
		platformUpdateWindow(&globalBackbuffer, window, renderer);

		for (int i = 0; i < MAX_CONTROLLERS; i++)
		{
			SDL_Gamepad *pad = GamepadHandles[i];
			if (!pad)
				continue;

			bool aButton = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);

			if (aButton)
			{
				yOffset += 2;
				SDL_RumbleGamepad(pad, 60000, 60000, 30);
			}
			else
			{

				SDL_RumbleGamepad(pad, 0, 0, 0);
			}
		}

		xOffset += 1;
	}

	// TODO(bruno): we are not freeing sdl renderer, sdl window and backbuffer
	// here because honestly the OS will handle this for us after this return 0.
	// but maybe we should revisit this?
	// --
	// nah, probably not

	return 0;
}
