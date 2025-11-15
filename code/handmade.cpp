#include "handmade.h"
#include "handmade_intrinsics.h"

global_variable real32 PI = 3.14159265359f;

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
		real32 sineValue = sin(gameState->tsine);
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

uint32 getTileUnchecked(World *world, Tilemap *tilemap, int32 tileX,
						int32 tileY) {
	assert(tileX < world->tilemapWidth);
	assert(tileY < world->tilemapHeight);
	return tilemap->tiles[tileY * world->tilemapWidth + tileX];
}

Tilemap *getTilemap(World *world, int32 tilemapX, int32 tilemapY) {
	assert(tilemapX < world->width);
	assert(tilemapY < world->height);
	assert(tilemapX >= 0);
	assert(tilemapY >= 0);
	return &world->tilemaps[tilemapY * world->width + tilemapX];
}

bool isTilemapPointEmpty(World *world, Tilemap *tilemap, int32 testTileX,
						 int32 testTileY) {
	bool empty = false;
	if (!tilemap) return false;

	if (testTileX >= 0 && testTileX < world->tilemapWidth && testTileY >= 0 &&
		testTileY < world->tilemapHeight) {
		uint32 tileID = getTileUnchecked(world, tilemap, testTileX, testTileY);
		if (tileID == 0) {
			empty = true;
		}
	}

	return empty;
}

inline CanonicalPosition getCanonicalPosition(World *world,
											  RawPosition rawPosition) {
	CanonicalPosition result;
	result.tilemapX = rawPosition.tilemapX;
	result.tilemapY = rawPosition.tilemapY;

	result.tileX = floorReal32ToInt32((rawPosition.x - world->upperLeftX) /
									  world->tileWidth);
	result.tileY = floorReal32ToInt32((rawPosition.y - world->upperLeftY) /
									  world->tileHeight);
	result.x = rawPosition.x - result.tileX * world->tileWidth;
	result.y = rawPosition.y - result.tileY * world->tileHeight;

	assert(result.x >= 0.0f && result.x < world->tileWidth);
	assert(result.y >= 0.0f && result.y < world->tileHeight);

	while (result.tileX < 0) {
		result.tileX += world->tilemapWidth;
		result.tilemapX -= 1;
	}
	while (result.tileX >= world->tilemapWidth) {
		result.tileX -= world->tilemapWidth;
		result.tilemapX += 1;
	}
	while (result.tileY < 0) {
		result.tileY += world->tilemapHeight;
		result.tilemapY -= 1;
	}
	while (result.tileY >= world->tilemapHeight) {
		result.tileY -= world->tilemapHeight;
		result.tilemapY += 1;
	}

	return result;
}

bool isWorldPointEmpty(World *world, RawPosition pos) {
	CanonicalPosition canonicalPosition = getCanonicalPosition(world, pos);

	Tilemap *tilemap = getTilemap(world, canonicalPosition.tilemapX,
								  canonicalPosition.tilemapY);
	return isTilemapPointEmpty(world, tilemap, canonicalPosition.tileX,
							   canonicalPosition.tileY);
}

void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input) {
	assert(sizeof(GameState) <= gameMemory->permanentStorageSize);

	GameState *gameState = (GameState *)gameMemory->permanentStorage;
	if (!gameMemory->isInitialized) {
		gameState->tsine = 0.0f;
		gameState->playerX = 150.0f;
		gameState->playerY = 150.0f;
		gameState->playerTilemapX = 0;
		gameState->playerTilemapY = 0;

		gameMemory->isInitialized = true;
	}

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
	tilemaps[0][1] = tilemaps[0][0];
	tilemaps[1][0] = tilemaps[0][0];
	tilemaps[1][1] = tilemaps[0][0];

	tilemaps[0][0].tiles = (uint32 *)tiles00;
	tilemaps[1][0].tiles = (uint32 *)tiles01;
	tilemaps[0][1].tiles = (uint32 *)tiles10;
	tilemaps[1][1].tiles = (uint32 *)tiles11;

	World world = {};
	world.width = 2;
	world.height = 2;
	world.tilemapWidth = TILEMAP_WIDTH;
	world.tilemapHeight = TILEMAP_HEIGHT;
	world.upperLeftX = 0.0f;
	world.upperLeftY = 0.0f;
	world.tileWidth = 60.0f;
	world.tileHeight = 60.0f;
	world.tilemaps = (Tilemap *)tilemaps;

	Tilemap *tilemap = getTilemap(&world, gameState->playerTilemapX,
								  gameState->playerTilemapY);

	real32 playerR = 0.0f;
	real32 playerG = 1.0f;
	real32 playerB = 1.0f;
	real32 playerWidth = 0.75f * world.tileWidth;
	real32 playerHeight = 0.75f * world.tileHeight;

	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
		} else {
			real32 speed = 200.0f * input->deltaTime;
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

			RawPosition playerPos = {};
			playerPos.tilemapX = gameState->playerTilemapX;
			playerPos.tilemapY = gameState->playerTilemapY;
			playerPos.x = newPlayerX;
			playerPos.y = newPlayerY;
			RawPosition playerLeft = playerPos;
			playerLeft.x = newPlayerX - (playerWidth / 2);
			RawPosition playerRight = playerPos;
			playerRight.x = newPlayerX + (playerWidth / 2);

			if (isWorldPointEmpty(&world, playerPos) &&
				isWorldPointEmpty(&world, playerLeft) &&
				isWorldPointEmpty(&world, playerRight)) {
				CanonicalPosition canonicalPosition =
					getCanonicalPosition(&world, playerPos);
				gameState->playerX = canonicalPosition.x +
									 canonicalPosition.tileX * world.tileWidth;
				gameState->playerY = canonicalPosition.y +
									 canonicalPosition.tileY * world.tileHeight;
				gameState->playerTilemapX = canonicalPosition.tilemapX;
				gameState->playerTilemapY = canonicalPosition.tilemapY;
			}
		}
	}

	gameOutputSound(soundBuffer,
					gameState); // TODO(bruno): Allow sample offsets
								// here for more robust platform options

	renderRectangle(backbuffer, 0, 0, backbuffer->width, backbuffer->height, 1,
					0, 1);

	for (int32 tileY = 0; tileY < world.tilemapHeight; tileY++) {
		for (int32 tileX = 0; tileX < world.tilemapWidth; tileX++) {

			uint32 tileID = getTileUnchecked(&world, tilemap, tileX, tileY);
			real32 gray = 0.5f;
			if (tileID == 1) {
				gray = 1.0f;
			}
			real32 minX = world.upperLeftX + tileX * world.tileWidth;
			real32 minY = world.upperLeftY + tileY * world.tileHeight;
			real32 maxX = minX + world.tileWidth;
			real32 maxY = minY + world.tileHeight;
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
