#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <stdlib.h>

#define global_variable static

global_variable int *globalBackbuffer;
global_variable bool globalRunning;
global_variable SDL_Window *globalSDLWindow;

const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

void processSDLEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
	}
}

int main(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		return -1;
	}

	globalBackbuffer = (int *)malloc(WINDOW_WIDTH * WINDOW_HEIGHT);
	globalSDLWindow = SDL_CreateWindow("Handmade Hero", WINDOW_WIDTH,
									   WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

	if (!globalBackbuffer || !globalSDLWindow)
	{
		return -1;
	}

	globalRunning = true;

	while (globalRunning)
	{
	}

	SDL_DestroyWindow(globalSDLWindow);
	SDL_Quit();

	return 0;
}
