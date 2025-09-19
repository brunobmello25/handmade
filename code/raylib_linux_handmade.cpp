#include "raylib.h"
#include <stdint.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

global_variable uint32_t *backbuffer;
global_variable int screenWidth;
global_variable int screenHeight;

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

void HandleResize(Texture2D *currentTexture, Image *currentImage)
{
	int currentWidth = GetScreenWidth();
	int currentHeight = GetScreenHeight();

	if (currentWidth != screenWidth || currentHeight != screenHeight)
	{
		screenWidth = currentWidth;
		screenHeight = currentHeight;

		if (currentTexture)
			UnloadTexture(*currentTexture);
		if (backbuffer)
			free(backbuffer);

		backbuffer =
			(uint32_t *)malloc(screenWidth * screenHeight * sizeof(uint32_t));

		currentImage->data = backbuffer;
		currentImage->width = screenWidth;
		currentImage->height = screenHeight;

		*currentTexture = LoadTextureFromImage(*currentImage);
	}
}

int main(int argc, char *argv[])
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Handmade Hero - Raylib");

	screenWidth = GetScreenWidth();
	screenHeight = GetScreenHeight();

	backbuffer =
		(uint32_t *)malloc(screenWidth * screenHeight * sizeof(uint32_t));

	Image image = {
		.data = backbuffer,
		.width = screenWidth,
		.height = screenHeight,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
	};

	Texture2D texture = LoadTextureFromImage(image);

	int xOffset = 0;
	int yOffset = 0;

	while (!WindowShouldClose())
	{

		HandleResize(&texture, &image);

		RenderWeirdGradient(backbuffer, screenWidth, screenHeight, xOffset,
							yOffset);

		UpdateTexture(texture, backbuffer);

		BeginDrawing();
		ClearBackground(BLACK);
		DrawTexture(texture, 0, 0, WHITE);
		EndDrawing();

		++xOffset;
	}

	UnloadTexture(texture);
	free(backbuffer);
	CloseWindow();

	return 0;
}
