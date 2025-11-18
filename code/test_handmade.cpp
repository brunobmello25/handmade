#include "handmade.cpp"
#include "test_framework.h"

World createTestWorld() {
	World world = {};
	world.width = 2;
	world.height = 2;
	world.tilemapWidth = 16;
	world.tilemapHeight = 9;
	world.tileSideInMeters = 1.4f;
	world.tileSideInPixels = 60;
	return world;
}

TEST(test_recanonicalizePosition_withinBounds) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 5;
	pos.tileY = 4;
	pos.x = 30.0f;
	pos.y = 25.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 0);
	EXPECT_EQ(result.tilemapY, 0);
	EXPECT_EQ(result.tileX, 5);
	EXPECT_EQ(result.tileY, 4);
	EXPECT_FLOAT_EQ(result.x, 30.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 25.0f, 0.01f);
}

TEST(test_recanonicalizePosition_xOverflow) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 5;
	pos.tileY = 4;
	pos.x = 70.0f;
	pos.y = 25.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 0);
	EXPECT_EQ(result.tilemapY, 0);
	EXPECT_EQ(result.tileX, 6);
	EXPECT_EQ(result.tileY, 4);
	EXPECT_FLOAT_EQ(result.x, 10.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 25.0f, 0.01f);
}

TEST(test_recanonicalizePosition_yOverflow) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 5;
	pos.tileY = 4;
	pos.x = 30.0f;
	pos.y = 125.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 0);
	EXPECT_EQ(result.tilemapY, 0);
	EXPECT_EQ(result.tileX, 5);
	EXPECT_EQ(result.tileY, 6);
	EXPECT_FLOAT_EQ(result.x, 30.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 5.0f, 0.01f);
}

TEST(test_recanonicalizePosition_tilemapXOverflow) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 15;
	pos.tileY = 4;
	pos.x = 70.0f;
	pos.y = 25.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 1);
	EXPECT_EQ(result.tilemapY, 0);
	EXPECT_EQ(result.tileX, 0);
	EXPECT_EQ(result.tileY, 4);
	EXPECT_FLOAT_EQ(result.x, 10.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 25.0f, 0.01f);
}

TEST(test_recanonicalizePosition_tilemapYOverflow) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 5;
	pos.tileY = 8;
	pos.x = 30.0f;
	pos.y = 70.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 0);
	EXPECT_EQ(result.tilemapY, 1);
	EXPECT_EQ(result.tileX, 5);
	EXPECT_EQ(result.tileY, 0);
	EXPECT_FLOAT_EQ(result.x, 30.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 10.0f, 0.01f);
}

TEST(test_recanonicalizePosition_xUnderflow) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 5;
	pos.tileY = 4;
	pos.x = -10.0f;
	pos.y = 25.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 0);
	EXPECT_EQ(result.tilemapY, 0);
	EXPECT_EQ(result.tileX, 4);
	EXPECT_EQ(result.tileY, 4);
	EXPECT_FLOAT_EQ(result.x, 50.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 25.0f, 0.01f);
}

TEST(test_recanonicalizePosition_exactBoundary) {
	World world = createTestWorld();

	CanonicalPosition pos = {};
	pos.tilemapX = 0;
	pos.tilemapY = 0;
	pos.tileX = 5;
	pos.tileY = 4;
	pos.x = 0.0f;
	pos.y = 0.0f;

	CanonicalPosition result = recanonicalizePosition(&world, pos);

	EXPECT_EQ(result.tilemapX, 0);
	EXPECT_EQ(result.tilemapY, 0);
	EXPECT_EQ(result.tileX, 5);
	EXPECT_EQ(result.tileY, 4);
	EXPECT_FLOAT_EQ(result.x, 0.0f, 0.01f);
	EXPECT_FLOAT_EQ(result.y, 0.0f, 0.01f);
}

int main() {
	printf("========================================\n");
	printf("Running Handmade Tests\n");
	printf("========================================\n");

	RUN_TEST(test_recanonicalizePosition_withinBounds);
	RUN_TEST(test_recanonicalizePosition_xOverflow);
	RUN_TEST(test_recanonicalizePosition_yOverflow);
	RUN_TEST(test_recanonicalizePosition_tilemapXOverflow);
	RUN_TEST(test_recanonicalizePosition_tilemapYOverflow);
	RUN_TEST(test_recanonicalizePosition_xUnderflow);
	RUN_TEST(test_recanonicalizePosition_exactBoundary);

	printTestSummary(&g_testContext);

	return g_testContext.failedTests > 0 ? 1 : 0;
}
