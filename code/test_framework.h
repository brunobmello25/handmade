#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "handmade.h"
#include <stdio.h>

struct TestContext {
	int totalTests;
	int passedTests;
	int failedTests;
	const char *currentTestName;
};

global_variable TestContext g_testContext = {};

#define TEST(name)                                                             \
	void name(TestContext *);                                                  \
	void name##_runner() {                                                     \
		g_testContext.currentTestName = #name;                                 \
		printf("\nRunning test: %s\n", #name);                                 \
		name(&g_testContext);                                                  \
	}                                                                          \
	void name(TestContext *ctx)

#define RUN_TEST(name)                                                         \
	do {                                                                       \
		name##_runner();                                                       \
	} while (0)

#define EXPECT_EQ(actual, expected)                                            \
	do {                                                                       \
		ctx->totalTests++;                                                     \
		if ((actual) == (expected)) {                                          \
			ctx->passedTests++;                                                \
			printf("  ✓ PASS: " #actual " == " #expected " (%d == %d)\n",      \
				   (int)(actual), (int)(expected));                            \
		} else {                                                               \
			ctx->failedTests++;                                                \
			printf("  ✗ FAIL: " #actual " == " #expected                       \
				   " (got %d, expected %d)\n",                                 \
				   (int)(actual), (int)(expected));                            \
		}                                                                      \
	} while (0)

#define EXPECT_FLOAT_EQ(actual, expected, epsilon)                             \
	do {                                                                       \
		ctx->totalTests++;                                                     \
		real32 diff = (actual) - (expected);                                   \
		if (diff < 0) diff = -diff;                                            \
		if (diff <= (epsilon)) {                                               \
			ctx->passedTests++;                                                \
			printf("  ✓ PASS: " #actual " ≈ " #expected " (%.2f ≈ %.2f)\n",    \
				   (float)(actual), (float)(expected));                        \
		} else {                                                               \
			ctx->failedTests++;                                                \
			printf("  ✗ FAIL: " #actual " ≈ " #expected                        \
				   " (got %.2f, expected %.2f, diff %.4f > %.4f)\n",           \
				   (float)(actual), (float)(expected), (float)diff,            \
				   (float)(epsilon));                                          \
		}                                                                      \
	} while (0)

void printTestSummary(TestContext *ctx) {
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:  %d\n", ctx->totalTests);
	printf("  Passed: %d\n", ctx->passedTests);
	printf("  Failed: %d\n", ctx->failedTests);
	printf("========================================\n");
	if (ctx->failedTests == 0) {
		printf("✓ All tests passed!\n");
	} else {
		printf("✗ Some tests failed.\n");
	}
}

#endif // TEST_FRAMEWORK_H
