#include "test.h"
#include "qsbr.h"
#include <stdio.h>

void test()
{
	/* Execute the test-dependent main loop. */
	while (tg->test_state == TEST_RUNNING) {
		testloop_body();
		quiescent_state(FUZZY);
		//printf("threads[%lu].rcount = %d\n", getTID(), threads[getTID()].rcount);
	}
}
