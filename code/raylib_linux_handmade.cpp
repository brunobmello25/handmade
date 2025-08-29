#include "raylib.h"
#include <stdint.h>

#define internal static
#define global_variable static
#define local_persist static

void RenderWeirdGradient(int width, int height, int xOffset, int yOffset)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint8_t blue = (x + xOffset);
			uint8_t green = (y + yOffset);
			uint8_t red = 0;

			Color color = {red, green, blue, 255};
			DrawPixel(x, y, color);
		}
	}
}

int main(int argc, char *argv[])
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Handmade Hero - Raylib");

	int xOffset = 0;
	int yOffset = 0;

	while (!WindowShouldClose())
	{
		BeginDrawing();

		RenderWeirdGradient(1280, 720, xOffset, yOffset);

		EndDrawing();

		++xOffset;
	}

	CloseWindow();

	return 0;
}
