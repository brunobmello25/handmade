#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

#include "handmade.cpp"

/*
	TODO(bruno): complete platform layer
	- Saved game locations
	- Getting a handle to our own executable file
	- Asset loading path
	- Threading (launch a thread)
	- Raw input (support multiple keyboards/mice)
	- Sleep/timeBeginPeriod (don't waste CPU time if not needed)
	- ClipCursor (multimonitor support)
	- Fullscreen support
	- WM_SETCURSOR (show/hide cursor)
	- QueryCancelAutoplay
	- WM_ACTIVATEAPP (for when we are not the active application)
	- Blit speed improvements (BitBlt? not sure what the raylib equivalent would
 be, if i even need it)
	- Hardware acceleration (OpenGL, Direct3D or both
	- GetKeyboardLayout (for international WASD support)

	Probably more stuff besides this
 * */

typedef struct
{
	uint32_t *pixels;
	Image image;
	Texture2D texture;
	int width;
	int height;
	int pitch;
} Backbuffer;

// Global state
global_variable Backbuffer globalBackbuffer;
global_variable int globalGamepad = 0;

// Animation state
global_variable int xOffset = 0;
global_variable int yOffset = 0;

// Backbuffer management
void InitializeBackbuffer(Backbuffer *backbuffer, int width, int height)
{
	backbuffer->width = width;
	backbuffer->height = height;
	backbuffer->pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));
	backbuffer->pitch = width * sizeof(uint32_t);

	backbuffer->image = (Image){
		.data = backbuffer->pixels,
		.width = width,
		.height = height,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
	};

	backbuffer->texture = LoadTextureFromImage(backbuffer->image);
}

void ResizeBackbuffer(Backbuffer *backbuffer, int newWidth, int newHeight)
{
	if (newWidth != backbuffer->width || newHeight != backbuffer->height)
	{
		UnloadTexture(backbuffer->texture);
		if (backbuffer->pixels)
			free(backbuffer->pixels);

		InitializeBackbuffer(backbuffer, newWidth, newHeight);
	}
}

void CleanupBackbuffer(Backbuffer *backbuffer)
{
	UnloadTexture(backbuffer->texture);
	if (backbuffer->pixels)
		free(backbuffer->pixels);
	backbuffer->pixels = NULL;
}

void HandleInput(int gamepad)
{
	if (!IsGamepadAvailable(gamepad))
		return;

	float leftStickX = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
	float leftStickY = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);

	float sensitivity = 3.0f;
	xOffset += (int)(leftStickX * sensitivity);
	yOffset += (int)(leftStickY * sensitivity);
}

void PrintGamepadInfo()
{
	for (int i = 0; i < 4; i++)
	{
		printf("Gamepad %d available: %s\n", i, GetGamepadName(i));
	}
}

int main(int argc, char *argv[])
{
	// Initialize window
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Handmade Hero - Raylib");

	// Setup backbuffer
	InitializeBackbuffer(&globalBackbuffer, GetScreenWidth(),
						 GetScreenHeight());

	PrintGamepadInfo();

	// Main loop
	while (!WindowShouldClose())
	{
		ResizeBackbuffer(&globalBackbuffer, GetScreenWidth(),
						 GetScreenHeight());

		HandleInput(globalGamepad);

		GameBackBuffer gameBackBuffer;
		gameBackBuffer.memory = globalBackbuffer.pixels;
		gameBackBuffer.width = globalBackbuffer.width;
		gameBackBuffer.height = globalBackbuffer.height;
		gameBackBuffer.pitch = globalBackbuffer.pitch;
		GameUpdateAndRender(&gameBackBuffer, xOffset, yOffset);

		UpdateTexture(globalBackbuffer.texture, globalBackbuffer.pixels);

		BeginDrawing();
		ClearBackground(BLACK);
		DrawTexture(globalBackbuffer.texture, 0, 0, WHITE);
		EndDrawing();
	}

	// Cleanup
	CleanupBackbuffer(&globalBackbuffer);
	CloseWindow();

	return 0;
}
