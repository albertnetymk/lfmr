#include "test.h"

void backoff_init()
{
    int i;
    for (i = 0; i < MAX_THREADS + 1; i++) {
        get_thread(i)->backoff_amount = 1;
    }
}

void backoff_reset()
{
    /* We want the initial backoff amount to be half what we had last
     * time. Since the first thing backoff_delay() does is double
     * the backoff amount, we divide by four.
     */
    int x = this_thread()->backoff_amount;
    x >>= 2;
    if (x == 0) {
        x = 1;
    }
    this_thread()->backoff_amount = x;
}

long backoff_delay()
{
        #ifdef USE_BACKOFF
    int i;
    long j;

    /* Double the backoff amount if doing so doesn't make us exceed
     * our limits. */
    this_thread()->backoff_amount <<= 2;
    while (this_thread()->backoff_amount > tg->nthreads) {
        this_thread()->backoff_amount >>= 2;
    }

    for (i = 0; i < this_thread()->backoff_amount; i++) {
        j = j * i;
    }

    if (j == this_thread()->backoff_amount) {
        return j;
    }
        #endif
    return 0;
}
