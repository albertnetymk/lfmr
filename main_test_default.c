#include "test.h"

void test()
{
	/* Execute the test-dependent main loop. */
	while (tg->test_state == TEST_RUNNING) {
		testloop_body();
	}
}
