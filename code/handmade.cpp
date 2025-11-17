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

inline void recanonicalizeCoord(World *world, int32 tileCount, int32 *tilemap,
								int32 *tile, real32 *relative) {
	int32 overflow =
		floorReal32ToInt32(*relative / (real32)world->tileSideInPixels);

	*tile += overflow;
	*relative -= overflow * world->tileSideInPixels;

	assert(*relative >= 0.0f);
	assert(*relative < world->tileSideInPixels);

	while (*tile < 0) {
		*tile += tileCount;
		*tilemap -= 1;
	}
	while (*tile >= tileCount) {
		*tile -= tileCount;
		*tilemap += 1;
	}
}

inline CanonicalPosition recanonicalizePosition(World *world,
												CanonicalPosition position) {

	CanonicalPosition result = position;

	recanonicalizeCoord(world, world->tilemapWidth, &result.tilemapX,
						&result.tileX, &result.x);
	recanonicalizeCoord(world, world->tilemapHeight, &result.tilemapY,
						&result.tileY, &result.y);

	return result;
}

bool isWorldPointEmpty(World *world, CanonicalPosition pos) {
	Tilemap *tilemap = getTilemap(world, pos.tilemapX, pos.tilemapY);
	return isTilemapPointEmpty(world, tilemap, pos.tileX, pos.tileY);
}

void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input) {
	assert(sizeof(GameState) <= gameMemory->permanentStorageSize);

	GameState *gameState = (GameState *)gameMemory->permanentStorage;
	if (!gameMemory->isInitialized) {
		gameState->tsine = 0.0f;
		gameState->playerPos.tilemapX = 0;
		gameState->playerPos.tilemapY = 0;
		gameState->playerPos.tileX = 2;
		gameState->playerPos.tileY = 2;
		gameState->playerPos.x = 5.0f;
		gameState->playerPos.y = 5.0f; // 5 pixels offset for now

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
	world.tilemaps = (Tilemap *)tilemaps;
	world.tileSideInMeters = 1.4f;
	world.tileSideInPixels = 60;

	Tilemap *tilemap = getTilemap(&world, gameState->playerPos.tilemapX,
								  gameState->playerPos.tilemapY);

	real32 playerR = 0.0f;
	real32 playerG = 1.0f;
	real32 playerB = 1.0f;
	real32 playerWidth = 0.75f * world.tileSideInPixels;
	real32 playerHeight = 0.75f * world.tileSideInPixels;

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

			CanonicalPosition newPosition = gameState->playerPos;
			newPosition.x += dPlayerX;
			newPosition.y += dPlayerY;
			newPosition = recanonicalizePosition(&world, gameState->playerPos);

			CanonicalPosition newLeft = newPosition;
			newLeft.x -= (playerWidth / 2);
			newLeft = recanonicalizePosition(&world, newLeft);
			CanonicalPosition newRight = newPosition;
			newRight.x += (playerWidth / 2);
			newRight = recanonicalizePosition(&world, newRight);

			if (isWorldPointEmpty(&world, newPosition) &&
				isWorldPointEmpty(&world, newLeft) &&
				isWorldPointEmpty(&world, newRight)) {
				gameState->playerPos = newPosition;
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
			real32 minX = tileX * world.tileSideInPixels;
			real32 minY = tileY * world.tileSideInPixels;
			real32 maxX = minX + world.tileSideInPixels;
			real32 maxY = minY + world.tileSideInPixels;
			renderRectangle(backbuffer, minX, minY, maxX, maxY, gray, gray,
							gray);
		}
	}

	real32 playerLeft = world.tileSideInPixels * gameState->playerPos.tileX +
						gameState->playerPos.x - 0.5f * playerWidth;
	real32 playerTop = gameState->playerPos.tileY + gameState->playerPos.y -
					   0.5f * playerHeight;
	real32 playerRight = playerLeft + playerWidth;
	real32 playerBottom = playerTop + playerHeight;
	renderRectangle(backbuffer, playerLeft, playerTop, playerRight,
					playerBottom, playerR, playerG, playerB);
}
