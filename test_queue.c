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
	int n = 100;
	/* Get all our randomness out of r and split it up. 
	 * Should have enough bits
	 */
	unsigned long r;
	unsigned long action;
	long key;
	struct queue *q = (struct queue *)tg->data_structure;

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
		key = r % tg->nkeys;

		if (action) {
			enqueue(q, key);
		} else {
			dequeue(q);
		}
	}

	/* Record another n operations. */
	this_thread()->op_count += n;
}

void usage(char *argv[])
{
  fprintf(stderr, "Usage: %s: nmilli nelements nthreads\n",
          argv[0]);
  exit(-1);
}

void setup_test(int argc, char **argv)
{
	int i, j;

	if (argc > 1) {
		tg->nmilli = strtoul(argv[1], NULL, 0);
	} else {
		tg->nmilli = 10000;
	}
	if (argc > 2) {
		tg->nelements = strtoul(argv[2], NULL, 0);
	} else {
		tg->nelements = 10;
	}
	tg->nkeys = tg->nelements * 2;
	if (tg->nkeys == 0) tg->nkeys = 1;
	if (argc > 3) {
		tg->nthreads = strtoul(argv[3], NULL, 0);
		if (tg->nthreads > MAX_THREADS) {
			printf("ERROR: Max threads is %lu\n", MAX_THREADS);
			exit(1);
		}
	} else {
		tg->nthreads = 2;
	}
	if (argc > 4) {
		usage(argv);
	}

	/* Initialize the random number generator. */
     init_Random();

	/* Initialize exponential backoff system. */
	backoff_init();

	/* Initialize the allocator. */
	init_allocator();

	/* Initialize the memory reclamation scheme. */
	mr_init();

	/* Initialize data_structure. */
	queue_init( (struct queue **) &tg->data_structure );

	/* Populate our queue.
	 * Don't use Random(). With a read-only workload, the list will never
	 * change, and that could really our tests up by giving one algorithm
	 * an easier list to search.*/
	for (i = 0, j = 0; i < tg->nelements; i++, j+= 2)
		enqueue((struct queue *)tg->data_structure, j);

	/* Summarize our parameters. */
	printf("%s: nmilli: %d nelements: %d nthreads: %d\n",
		  argv[0], tg->nmilli, tg->nelements, tg->nthreads);

}
