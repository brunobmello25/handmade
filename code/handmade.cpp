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

uint32 getTileUnchecked(World *world, Chunk *chunk, uint32 tileX,
						uint32 tileY) {
	assert(chunk);
	assert(tileX < world->chunkSize);
	assert(tileY < world->chunkSize);
	return chunk->tiles[tileY * world->chunkSize + tileX];
}

inline void recanonicalizeCoord(World *world, uint32 *tile,
								real32 *tileOffset) {
	// TODO(bruno): figure out a way to do this without division and
	// multiplication, because this can end up rounding back to the same value

	// TODO(bruno): add bounds checking to prevent wraping

	int32 offset =
		floorReal32ToInt32(*tileOffset / (real32)world->tileSideInMeters);
	*tile += offset;
	*tileOffset -= ((real32)offset * (real32)world->tileSideInMeters);

	assert(*tileOffset <= world->tileSideInMeters);
	assert(*tileOffset >= 0);
}

inline WorldPosition recanonicalizePosition(World *world,
											WorldPosition position) {
	WorldPosition result = position;

	recanonicalizeCoord(world, &result.tileX, &result.tileOffsetX);
	recanonicalizeCoord(world, &result.tileY, &result.tileOffsetY);

	return result;
}

ChunkPosition worldPosToChunkPos(World *world, WorldPosition worldPosition) {
	ChunkPosition result = {};

	result.chunkX = worldPosition.tileX >> world->chunkShift;
	result.chunkY = worldPosition.tileY >> world->chunkShift;
	result.tileX = worldPosition.tileX & world->chunkMask;
	result.tileY = worldPosition.tileY & world->chunkMask;

	return result;
}

bool isChunkPointEmpty(World *world, Chunk *chunk,
					   ChunkPosition chunkPosition) {
	uint32 tileID = getTileUnchecked(world, chunk, chunkPosition.tileX,
									 chunkPosition.tileY);
	return (tileID == 0);
}

Chunk *getChunk(World *world, ChunkPosition chunkPosition) {
	// FIXME(bruno): only one chunk for now
	assert(chunkPosition.chunkX == 0);
	assert(chunkPosition.chunkY == 0);
	return &world->chunks[0];
}

bool isWorldPointEmpty(World *world, WorldPosition pos) {
	ChunkPosition chunkPosition = worldPosToChunkPos(world, pos);
	Chunk *chunk = getChunk(world, chunkPosition);
	return isChunkPointEmpty(world, chunk, chunkPosition);
}

void gameUpdateAndRender(GameMemory *gameMemory, GameBackbuffer *backbuffer,
						 GameSoundBuffer *soundBuffer, GameInput *input) {
	assert(sizeof(GameState) <= gameMemory->permanentStorageSize);

	GameState *gameState = (GameState *)gameMemory->permanentStorage;
	if (!gameMemory->isInitialized) {
		gameState->tsine = 0.0f;
		gameState->playerPos.tileX = 3;
		gameState->playerPos.tileY = 3;
		gameState->playerPos.tileOffsetX = 0.1f;
		gameState->playerPos.tileOffsetY = 0.1f; // 5 pixels offset for now

		gameMemory->isInitialized = true;
	}

	// clang-format off
	#define CHUNK_SIZE 256
	uint32 tiles00[CHUNK_SIZE][CHUNK_SIZE] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	};
	// clang-format on

	Chunk chunks[1][1] = {};
	chunks[0][0].tiles = (uint32 *)tiles00;

	World world = {};
	world.chunkSize = CHUNK_SIZE;
	world.chunks = (Chunk *)chunks;
	world.tileSideInMeters = 1.4f;
	world.tileSideInPixels = 60;
	world.metersToPixels =
		((real32)world.tileSideInPixels / world.tileSideInMeters);
	world.lowerLeftX = 0;
	world.lowerLeftY = backbuffer->height;
	world.chunkShift = 8;
	world.chunkMask = 0xFF;

	real32 playerR = 0.0f;
	real32 playerG = 1.0f;
	real32 playerB = 1.0f;
	real32 playerHeight = world.tileSideInMeters;
	real32 playerWidth = 0.75f * playerHeight;

	for (size_t i = 0; i < arraylength(input->controllers); i++) {
		GameControllerInput *controller = gameGetController(input, i);

		if (controller->isAnalog) {
		} else {
			real32 speed = 5.0f * input->deltaTime;
			real32 dPlayerX = 0.0f;
			real32 dPlayerY = 0.0f;
			if (controller->moveDown.endedDown) dPlayerY = -1.0f;
			if (controller->moveUp.endedDown) dPlayerY = 1.0f;
			if (controller->moveLeft.endedDown) dPlayerX = -1.0f;
			if (controller->moveRight.endedDown) dPlayerX = 1.0f;
			dPlayerX *= speed;
			dPlayerY *= speed;

			WorldPosition newPosition = gameState->playerPos;
			newPosition.tileOffsetX += dPlayerX;
			newPosition.tileOffsetY += dPlayerY;
			newPosition = recanonicalizePosition(&world, newPosition);

			WorldPosition newLeft = newPosition;
			newLeft.tileOffsetX -= (playerWidth / 2);
			newLeft = recanonicalizePosition(&world, newLeft);
			WorldPosition newRight = newPosition;
			newRight.tileOffsetX += (playerWidth / 2);
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

	Chunk chunk = world.chunks[0];
	for (uint32 row = 0; row < world.chunkSize; row++) {
		for (uint32 column = 0; column < world.chunkSize; column++) {

			uint32 tileID = getTileUnchecked(&world, &chunk, column, row);
			real32 gray = 0.5f;
			if (tileID == 1) {
				gray = 1.0f;
			}

#if HANDMADE_INTERNAL
			if (column == gameState->playerPos.tileX &&
				row == gameState->playerPos.tileY) {
				gray = 0.0f;
			}
#endif

			real32 minX = world.lowerLeftX + column * world.tileSideInPixels;
			real32 minY = world.lowerLeftY - row * world.tileSideInPixels;
			real32 maxX = minX + world.tileSideInPixels;
			real32 maxY = minY - world.tileSideInPixels;
			renderRectangle(backbuffer, minX, maxY, maxX, minY, gray, gray,
							gray);
		}
	}

	real32 playerLeft =
		world.lowerLeftX + world.tileSideInPixels * gameState->playerPos.tileX +
		world.metersToPixels * gameState->playerPos.tileOffsetX -
		0.5f * world.metersToPixels * playerWidth;
	real32 playerRight = playerLeft + world.metersToPixels * playerWidth;

	real32 playerTop = world.lowerLeftY -
					   world.tileSideInPixels * gameState->playerPos.tileY -
					   world.metersToPixels * gameState->playerPos.tileOffsetY -
					   0.5f * world.metersToPixels * playerHeight;
	real32 playerBottom = playerTop + world.metersToPixels * playerHeight;

	renderRectangle(backbuffer, playerLeft, playerTop, playerRight,
					playerBottom, playerR, playerG, playerB);
}
