#include <SDL3/SDL.h>
#include <stdlib.h>

#define global_variable static

struct SDLBackbuffer
{
	int width;
	int height;
	void *memory;
	SDL_Texture *texture;
};

global_variable SDLBackbuffer globalBackbuffer;
global_variable bool globalRunning;

bool processSDLEvents()
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
	}
	return true;
}

// function that gets called everytime the window size changes, and also
// at first time, in order to allocate the backbuffer with proper dimensions
void resizeSDLBackbuffer(SDLBackbuffer *backbuffer, SDL_Renderer *renderer,
						 int width, int height)
{
	int bytesPerPixel = 4;

	if (backbuffer->memory)
		free(backbuffer->memory);

	if (backbuffer->texture)
		SDL_DestroyTexture(backbuffer->texture);

	backbuffer->width = width;
	backbuffer->height = height;
	backbuffer->memory = malloc(width * height * bytesPerPixel);
	backbuffer->texture =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, width, height);
}

int main(void)
{
	int initialWidth = 1920 / 2;
	int initialHeight = 1080 / 2;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		return -1;
	SDL_Window *window = SDL_CreateWindow("Handmade Hero", initialWidth,
										  initialHeight, SDL_WINDOW_RESIZABLE);
	if (!window) // TODO(bruno): proper error handling
		return -1;

	SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
	if (!renderer) // TODO(bruno): proper error handling
		return -1;

	globalBackbuffer = {};
	resizeSDLBackbuffer(&globalBackbuffer, renderer, initialWidth,
						initialHeight);
	globalRunning = true;

	while (globalRunning)
	{
		globalRunning = processSDLEvents();
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
