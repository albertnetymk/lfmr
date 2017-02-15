#include "test.h"
#include "nebr.h"

void test()
{
	/* Execute the test-dependent main loop. */
	while (tg->test_state == TEST_RUNNING) {
		nebr_set_active();
		testloop_body();
		nebr_set_inactive();
	}
}
