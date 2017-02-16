#include "test.h"
#include "list.h"
#include "mr.h"
#include "random.h"
#include "allocator.h"
#include <stdio.h>

void testloop_body()
{
    uint8_t n_search = 100 - n_update;
    const int n = 100;
    /* Get all our randomness out of r and split it up.
     * Should have enough bits
     */
    unsigned long r;
    unsigned long action;
    unsigned long ins_del;
    unsigned long arange = 101;
    unsigned long amask = (1 << 10) - 1;
    long key;

    struct list *l = (struct list*)global_ds;

    for (int i = 0; i < n; ++i) {
        r = Random();
        ins_del = r & 1;
        r >>= 1;
        action = ((r & amask) % arange);
        r >>= 10;
        key = r % n_keys;

        if (action <= n_search) {
            search(l, key);
        } else if (ins_del == 0) {
            delete(l, key);
        } else {
            insert(l, key);
        }
    }

    /* Record another n operations. */
    threads[thread_id].ops += n;
}

void setup_test(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s: nmilli update_percent nelements nthreads\n",
                argv[0]);
        exit(-1);
    }

    n_ms = (uint32_t)atoi(argv[1]);
    n_update = (uint8_t)atoi(argv[2]);
    n_elements = (uint32_t)atoi(argv[3]);
    n_threads = (uint32_t)atoi(argv[4]);
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
    list_init((struct list**)&global_ds);

    for (uint32_t i = 0, j = 0; i < n_elements; i++, j += 2) {
        insert((struct list*)global_ds, j);
    }
}
