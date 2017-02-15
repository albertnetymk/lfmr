#include "test.h"
#include "list.h"
#include "mr.h"
#include "random.h"
#include <stdio.h>

void testloop_body()
{
	int i;
	int n = 100;
	/* Get all our randomness out of r and split it up. 
	 * Should have enough bits
	 */
	unsigned long r;
	unsigned long action;
	unsigned long ins_del;
	unsigned long arange = tg->nsearch + tg->nmodify + 1;
	unsigned long amask = (1<<10) - 1;
	long key;

	struct list *l = (struct list *)tg->data_structure;

	for (i = 0; i < n; i++) {
		r = Random();
		ins_del = r & 1;
		r >>= 1;
		action = ((r & amask) % arange);
		r >>= 10;
		key = r % tg->nkeys;

		if (action <= tg->nsearch) {
			search(l, key);
		} else if (ins_del == 0) {
			delete(l, key);
		} else {
			insert(l, key);
		}
	}

	/* Record another n operations. */
	this_thread()->op_count += n;
}

void usage(char *argv[])
{
  fprintf(stderr, "Usage: %s: nmilli search/total nelements nthreads\n",
          argv[0]);
  exit(-1);
}


void parse_update_modify(char *cp, char **argv)
{
	char *denom;
	
	tg->nmodify = strtol(cp, &denom, 0);
	if (tg->nmodify < 0) {
		fprintf(stderr, "update fraction cannot be negative");
		usage(argv);
	}
	if (denom[0] == '\0') {
		tg->nsearch = 100 - tg->nmodify;
	} else if (denom[0] == '/') {
		tg->nsearch = strtol(&(denom[1]), NULL, 0) - tg->nmodify;
		if (tg->nsearch < 0) {
			fprintf(stderr, "update fraction cannot be > 1.0");
			usage(argv);
		}
	} else {
		usage(argv);
	}

	/* Range-checking -- necessary since we're using 10 bits of
	 * a random number during the tests, above.
	 */
	if (tg->nsearch+tg->nmodify > 1024) {
		fprintf(stderr, "nsearch + nmodify must be <= 1024");
		usage(argv);
	}
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
		parse_update_modify(argv[2], argv);
	} else {
		tg->nsearch = 1000;
		tg->nmodify = 1000;
	}
	if (argc > 3) {
		tg->nelements = strtoul(argv[3], NULL, 0);
	} else {
		tg->nelements = 10;
	}
	tg->nkeys = tg->nelements * 2;
	if (tg->nkeys == 0) tg->nkeys = 1;
	if (argc > 4) {
		tg->nthreads = strtoul(argv[4], NULL, 0);
		if (tg->nthreads > MAX_THREADS) {
			printf("ERROR: Max threads is %lu\n", MAX_THREADS);
			exit(1);
		}
	} else {
		tg->nthreads = 2;
	}
	if (argc > 5) {
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
	list_init( (struct list **) &tg->data_structure );

	/* Populate our list. 
	 * Don't use Random(). With a read-only workload, the list will never
	 * change, and that could really our tests up by giving one algorithm
	 * an easier list to search.*/
	for (i = 0, j = 0; i < tg->nelements; i++, j+= 2)
		insert((struct list *)tg->data_structure, j);

	/* Summarize our parameters. */
	printf("%s: nmilli: %d update/total: %d/%d nelements: %d nthreads: %d\n",
		  argv[0], tg->nmilli, tg->nmodify, tg->nsearch + tg->nmodify, 
		  tg->nelements, tg->nthreads);

}
