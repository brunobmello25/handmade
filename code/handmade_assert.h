#ifndef HANDMADE_ASSERT_H

#if HANDMADE_SLOW
#define assert(expression)                                                     \
	if (!(expression)) {                                                       \
		*(int *)0 = 0;                                                         \
	}
#else
#define assert(expression)
#endif

#define HANDMADE_ASSERT_H
#endif
