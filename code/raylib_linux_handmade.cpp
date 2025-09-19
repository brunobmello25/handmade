#include "raylib.h"
#include <stdint.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

typedef struct
{
	uint32_t *pixels;
	Image image;
	Texture2D texture;
	int width;
	int height;
} Backbuffer;

// Global state
global_variable Backbuffer globalBackbuffer;

// Rendering
void RenderWeirdGradient(uint32_t *pixels, int width, int height, int xOffset,
						 int yOffset)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint8_t blue = (x + xOffset);
			uint8_t green = (y + yOffset);
			uint8_t red = 0;

			uint32_t pixel = (0xFF << 24) | (blue << 16) | (green << 8) | red;
			pixels[y * width + x] = pixel;
		}
	}
}

// Backbuffer management
void InitializeBackbuffer(Backbuffer *backbuffer, int width, int height)
{
	backbuffer->width = width;
	backbuffer->height = height;
	backbuffer->pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

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

int main(int argc, char *argv[])
{
	// Initialize window
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Handmade Hero - Raylib");

	// Setup backbuffer
	InitializeBackbuffer(&globalBackbuffer, GetScreenWidth(),
						 GetScreenHeight());

	// Animation state
	int xOffset = 0;
	int yOffset = 0;

	// Main loop
	while (!WindowShouldClose())
	{
		ResizeBackbuffer(&globalBackbuffer, GetScreenWidth(),
						 GetScreenHeight());

		RenderWeirdGradient(globalBackbuffer.pixels, globalBackbuffer.width,
							globalBackbuffer.height, xOffset, yOffset);

		UpdateTexture(globalBackbuffer.texture, globalBackbuffer.pixels);

		BeginDrawing();
		ClearBackground(BLACK);
		DrawTexture(globalBackbuffer.texture, 0, 0, WHITE);
		EndDrawing();

		++xOffset;
	}

	// Cleanup
	CleanupBackbuffer(&globalBackbuffer);
	CloseWindow();

	return 0;
}
