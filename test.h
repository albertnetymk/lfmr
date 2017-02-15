#ifndef TEST_H
#define TEST_H

#include <stdlib.h>
#include "node.h"
#include "mcslock.h"

/****************************************************************************
 *
 *   Definitions of constants.
 *
 ****************************************************************************/

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MAX_THREADS 32
#define NUM_TRIALS 5
#define NS_PER_S 1000000000
#define CACHESIZE 128           /* Should cover most machines. */

#define N_EPOCHS 3

/****************************************************************************
 *
 *   Data structures.
 *
 ****************************************************************************/

struct per_thread {
	double op_count;

	unsigned long randseed;

	/* We don't want to punish MR schemes or algorithms with extra 
	 * dereferences, so just include data for all of them */
	
	/* For slab allocator. */
	node_t *freelist;
	node_t *freelist2;
	unsigned long freelist_count;

	/* For exponential backoff. */
	unsigned long backoff_amount;

	/* For using mcs locks (can only use one at a time unless we give a 
	 * thread extra nodes.
	 */
	qnode_t mynode;

	/* 
	 * Private per-thread node pointers for lock-free linked list:
	 *   prev : holds the old address of cur for CAS operations 
	 *   cur  : the node we want to do something with 
	 *          (delete, insert after, etc.)
	 *   next : the successor of cur
	 */
	node_t **prev;
	node_t *next;
	node_t *cur;

	/* SMR per-thread data:
	 *  rlist: retired list
	 *  rcount: retired count
	 */
	node_t *rlist;
	node_t **plist;

	/* Count of logically deleted nodes awaiting reclamation; used by
	 * all MR schemes. 
	 */
	unsigned long rcount;

	/* QSBR per-thread data:
	 *  flag: local variable to spin on.
	 *  n_inflight: debugging info
	 *  qsbr_{currlist,midlist,nxtlist}: limbo lists. Due to the fuzzy 
	 *  barrier, we need three lists instead of two.
	 */
	volatile unsigned long flag __attribute__ ((__aligned__ (CACHESIZE)));
	node_t *qsbr_currlist;
	node_t *qsbr_midlist;
	node_t *qsbr_nxtlist;

	/* EBR per-thread data:
	 *  limbo_list: three lists of nodes awaiting physical deletion, one
	 *              for each epoch
	 *  in_critical: flag telling us whether we're in a critical section
	 *               with respect to memory reclamation
	 *  entries_since_update: the number of times we have entered a critical 
	 *                        section in the current epoch since trying to
	 *                        update the global epoch
	 *  epoch: the local epoch
	 */
	node_t *limbo_list [N_EPOCHS];
	int in_critical;
	int entries_since_update;
	int epoch;
} __attribute__ ((__aligned__ (CACHESIZE)));

/****************************************************************************
 *
 *   Variables common to all tests.
 *
 ****************************************************************************/

struct test_globals {

	/* Global freelist of nodes. */
	node_t *global_freelist;

	/* Global flag set by the parent thread. Controls when the worker threads
	 * start and stop.
	 */
	volatile int test_state;
#define TEST_NOT_STARTED 0
#define TEST_RUNNING 1
#define TEST_OVER 2

	/* Duration of the test, in milliseconds. */
	unsigned long nmilli;

	/* Number of threads for the test. */
	unsigned long nthreads;
	
	/* Number of threads which have completed execution. */
	unsigned long n_arrived;

	/* A pointer to the data structure on which the threads will work. We're 
	 * using  C's weak typing as poor man's polymorphism.
	 */
	void *data_structure;

	/* Per-thread data; MAX_THREADS lets us statically allocate it. 
	 * Note -- the "+1" is for the parent thread.
	 */
	struct per_thread threads [MAX_THREADS+1];

	/* Parameters for queues. */
	unsigned long nelements;
	unsigned long nkeys;
	/* Add for linked lists. */
	unsigned long nsearch;
	unsigned long nmodify;
	/* Add for hash tables. */
	unsigned long nbuckets;

	unsigned long out_of_memory;
};

struct test_globals *tg;

/* The thread's TID. */
unsigned long g_tid;

/****************************************************************************
 *
 *   Utility macros and functions.
 *
 ****************************************************************************/

#define getTID() (g_tid)
#define this_thread() (&(tg->threads[g_tid]))
#define get_thread(n) (&(tg->threads[(n)]))

#define spin() {do {} while (1);}

#ifdef YIELD
#define cond_yield() sched_yield()
#else
#define cond_yield() ;
#endif

/****************************************************************************
 *
 *   Interface to test functions.
 *
 ****************************************************************************/

/* Sets up the test, in a test-depedent way. Parses the argv. Instantiates 
 * data_structure to point to the right data structure.
 */
void setup_test(int argc, char **argv);

/* Different data structures -- ie. linked list, queue -- must  provide 
 * implementations of this. It's the workhorse function of the performance test.
 */
void testloop_body();

#endif
