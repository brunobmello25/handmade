#include "test_framework.h"

TEST(example) { EXPECT_EQ(1, 1); }

int main() {
	RUN_TEST(example);

	printTestSummary(&g_testContext);

	return g_testContext.failedTests > 0 ? 1 : 0;
}
