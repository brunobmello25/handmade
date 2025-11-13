#include "handmade.h"
#include <math.h>

global_variable real32 PI = 3.14159265359f;

int32 roundReal32ToInt32(real32 value) { return (int32)(value + 0.5f); }
uint32 roundReal32ToUInt32(real32 value) { return (uint32)(value + 0.5f); }
int32 truncateReal32ToInt32(real32 value) { return (int32)(value); }
uint32 truncateReal32ToUInt32(real32 value) { return (uint32)(value); }

void renderRectangle(GameBackbuffer *buffer, real32 minXf, real32 minYf,
					 real32 maxXf, real32 maxYf, real32 R, real32 G, real32 B) {
	int32 minX = roundReal32ToInt32(minXf);
	int32 minY = roundReal32ToInt32(minYf);
	int32 maxX = roundReal32ToInt32(maxXf);
	int32 maxY = roundReal32ToInt32(maxYf);
	if (minX < 0) minX = 0;
	if (minY < 0) minY = 0;
	if (maxX > buffer->width) maxX = buffer->width;
	if (maxY > buffer->height) maxY = buffer->height;

	uint32 color =
		(((uint32)255.0f << 24) | (roundReal32ToUInt32(R * 255.0f) << 16) |
		 (roundReal32ToUInt32(G * 255.0f) << 8) |
		 (roundReal32ToUInt32(B * 255.0f) << 0));

	size_t bytesPerPixel = sizeof(uint32);

	uint8 *row =
		(uint8 *)buffer->memory + minX * bytesPerPixel + minY * buffer->pitch;

	for (int y = minY; y < maxY; y++) {
		uint32 *pixel = (uint32 *)row;
		for (int x = minX; x < maxX; x++) {
			*pixel++ = color;
		}
		row += buffer->pitch;
	}
}

void gameOutputSound(GameSoundBuffer *soundBuffer, GameState *gameState) {
	if (soundBuffer->sampleCount == 0) return;

	int toneHz = 256;
	int wavePeriod = soundBuffer->sampleRate / toneHz;

	int16 *sampleOut = soundBuffer->samples;

	for (int i = 0; i < soundBuffer->sampleCount; i++) {
		// TODO(bruno): ditch this compile-time flag once we stop debugging
		// audio with a sine wave
#if ENABLE_SINE_WAVE
		int toneVolume = 3000;
		real32 sineValue = sinf(gameState->tsine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
#else
		int16 sampleValue = 0;
#endif
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		gameState->tsine += 2.0f * PI * 1.0f / (real32)wavePeriod;
		if (gameState->tsine > 2.0f * PI) {
			gameState->tsine -= 2.0f * PI;
		}
	}
}

uint32 getTileUnchecked(Tilemap *tilemap, uint32 tileX, uint32 tileY) {
	assert(tileX < tilemap->width);
	assert(tileY < tilemap->height);
	return tilemap->tiles[tileY * tilemap->width + tileX];
}

Tilemap *getTilemap(World *world, uint32 tilemapX, uint32 tilemapY) {
	assert(tilemapX < world->width);
	assert(tilemapY < world->height);
	return &world->tilemaps[tilemapY * world->width + tilemapX];
}

bool isTilemapPointEmpty(Tilemap *tilemap, real32 testX, real32 testY) {
	bool empty = false;

	int32 testTileX = truncateReal32ToInt32((testX - tilemap->upperLeftX) /
											tilemap->tileWidth);
	int32 testTileY = truncateReal32ToInt32((testY - tilemap->upperLeftY) /
											tilemap->tileHeight);

	if (testTileX >= 0 && testTileX < tilemap->width && testTileY >= 0 &&
		testTileY < tilemap->height) {
		uint32 tileID = getTileUnchecked(tilemap, testTileX, testTileY);
		if (tileID == 0) {
			empty = true;
		}
	}

	return empty;
}

bool isWorldPointEmpty(World *world, real32 testX, real32 testY) {
	assert(!"Not implemented yet");
	return false;
}

void renderPlayer(GameBackbuffer *backbuffer, GameState *gameState,
				  real32 tileWidth, real32 tileHeight) {}

void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input) {
	assert(sizeof(GameState) <= gameMemory->permanentStorageSize);

#define TILEMAP_WIDTH 16
#define TILEMAP_HEIGHT 9
	uint32 tiles00[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
	};
	uint32 tiles01[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
		{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	};
	uint32 tiles10[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
	};
	uint32 tiles11[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
		{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	};

	Tilemap tilemaps[2][2] = {};
	tilemaps[0][0].width = TILEMAP_WIDTH;
	tilemaps[0][0].height = TILEMAP_HEIGHT;
	tilemaps[0][0].upperLeftX = 0.0f;
	tilemaps[0][0].upperLeftY = 0.0f;
	tilemaps[0][0].tileWidth = 60.0f;
	tilemaps[0][0].tileHeight = 60.0f;
	tilemaps[0][1] = tilemaps[0][0];
	tilemaps[1][0] = tilemaps[0][0];
	tilemaps[1][1] = tilemaps[0][0];

	tilemaps[0][0].tiles = (uint32 *)tiles00;
	tilemaps[0][1].tiles = (uint32 *)tiles01;
	tilemaps[1][0].tiles = (uint32 *)tiles10;
	tilemaps[1][1].tiles = (uint32 *)tiles11;

	Tilemap tilemap = tilemaps[0][0];

	World world = {};
	world.tilemaps = (Tilemap *)tilemaps;

	real32 playerR = 0.0f;
	real32 playerG = 1.0f;
	real32 playerB = 1.0f;
	real32 playerWidth = 0.75f * tilemap.tileWidth;
	real32 playerHeight = 0.75f * tilemap.tileHeight;

	GameState *gameState = (GameState *)gameMemory->permanentStorage;
	if (!gameMemory->isInitialized) {
		gameState->tsine = 0.0f;
		gameState->playerX = 150.0f;
		gameState->playerY = 150.0f;

		gameMemory->isInitialized = true;
	}

	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
		} else {
			real32 speed = 100.0f * input->deltaTime;
			real32 dPlayerX = 0.0f;
			real32 dPlayerY = 0.0f;
			if (controller->moveDown.endedDown) dPlayerY = 1.0f;
			if (controller->moveUp.endedDown) dPlayerY = -1.0f;
			if (controller->moveLeft.endedDown) dPlayerX = -1.0f;
			if (controller->moveRight.endedDown) dPlayerX = 1.0f;
			dPlayerX *= speed;
			dPlayerY *= speed;

			real32 newPlayerX = gameState->playerX + dPlayerX;
			real32 newPlayerY = gameState->playerY + dPlayerY;

			if (isTilemapPointEmpty(&tilemap, newPlayerX, newPlayerY) &&
				isTilemapPointEmpty(&tilemap, newPlayerX - (playerWidth / 2),
									newPlayerY) &&
				isTilemapPointEmpty(&tilemap, newPlayerX + (playerWidth / 2),
									newPlayerY)) {
				gameState->playerX = newPlayerX;
				gameState->playerY = newPlayerY;
			}
		}
	}

	gameOutputSound(soundBuffer,
					gameState); // TODO(bruno): Allow sample offsets
								// here for more robust platform options

	renderRectangle(backbuffer, 0, 0, backbuffer->width, backbuffer->height, 1,
					0, 1);

	for (int32 tileY = 0; tileY < tilemap.height; tileY++) {
		for (int32 tileX = 0; tileX < tilemap.width; tileX++) {

			uint32 tileID = getTileUnchecked(&tilemap, tileX, tileY);
			real32 gray = 0.5f;
			if (tileID == 1) {
				gray = 1.0f;
			}
			real32 minX = tilemap.upperLeftX + tileX * tilemap.tileWidth;
			real32 minY = tilemap.upperLeftY + tileY * tilemap.tileHeight;
			real32 maxX = minX + tilemap.tileWidth;
			real32 maxY = minY + tilemap.tileHeight;
			renderRectangle(backbuffer, minX, minY, maxX, maxY, gray, gray,
							gray);
		}
	}

	real32 playerLeft = gameState->playerX - (playerWidth / 2);
	real32 playerTop = gameState->playerY - (playerHeight);
	real32 playerRight = playerLeft + playerWidth;
	real32 playerBottom = playerTop + playerHeight;
	renderRectangle(backbuffer, playerLeft, playerTop, playerRight,
					playerBottom, playerR, playerG, playerB);
}
