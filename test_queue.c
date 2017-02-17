#include "test.h"
#include "queue.h"
#include "mr.h"
#include "random.h"
#include "backoff.h"
#include <stdio.h>

#include "allocator.h"
#include "arch/atomic.h"

void testloop_body()
{
    int i;
    const int n = 100;
    /* Get all our randomness out of r and split it up.
     * Should have enough bits
     */
    unsigned long r;
    unsigned long action;
    long key;
    struct queue *q = (struct queue*)global_ds;

    for (i = 0; i < n; i++) {
        /* Using Random(), we weren't getting a uniform mix of enqueues
        * and dequeues -- the queue kept growing. Hence hardwire it. */
        // Hmm, seems to work with random... keep it!
        // Test used: try with 1000 or 10000 duration -> same answer.
        // Must have been a problem with my LFRC implementation when
        // being debugged. :)
        r = Random();
        //r = i;
        action = r & 1;
        r >>= 1;
        key = r % n_keys;

        if (action) {
            enqueue(q, key);
        } else {
            dequeue(q);
        }
    }

    /* Record another n operations. */
    threads[thread_id].ops += n;
}

void setup_test(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s: nmilli nelements nthreads\n",
                argv[0]);
        exit(-1);
    }

    n_ms = (uint32_t)atoi(argv[1]);
    n_elements = (uint32_t)atoi(argv[2]);
    n_threads = (uint32_t)atoi(argv[3]);
    n_keys = n_elements * 2;

    /* Initialize the random number generator. */
    init_Random();

    /* Initialize exponential backoff system. */
    backoff_init();

    /* Initialize the allocator. */
    init_allocator();

    /* Initialize the memory reclamation scheme. */
    mr_init();

    /* Initialize data_structure. */
    queue_init((struct queue**)&global_ds);

    /* Populate our queue.
     * Don't use Random(). With a read-only workload, the list will never
     * change, and that could really our tests up by giving one algorithm
     * an easier list to search.*/
    for (uint32_t i = 0, j = 0; i < n_elements; i++, j += 2) {
        enqueue((struct queue*)global_ds, j);
    }
}
