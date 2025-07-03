#include "SDL_events.h"
#include "SDL_render.h"
#include "SDL_video.h"
#include <SDL.h>
#include <cstdio>

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
        case SDL_WINDOWEVENT_RESIZED:
        {
          printf("SDL_WINDOWEVENT_RESIZEd: %d x %d\n", Event->window.data1,
                 Event->window.data2);
        }
        break;
        case SDL_WINDOWEVENT_EXPOSED:
        {
          static bool IsWhite = true;
          SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
          SDL_Renderer *Renderer = SDL_GetRenderer(Window);

          if (IsWhite)
          {
            SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 255);
            IsWhite = false;
          }
          else
          {
            SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
            IsWhite = true;
          }
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

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    // TODO: Handle error
  }

  SDL_Window *Window;
  Window =
      SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);

  if (Window)
  {
    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

    if (Renderer)
    {
      while (true)
      {
        SDL_Event event;
        SDL_WaitEvent(&event);

        if (HandleEvent(&event))
        {
          break;
        }

        SDL_RenderClear(Renderer);
        SDL_RenderPresent(Renderer);
      }
    }
  }

  return (0);
}
