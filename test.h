#ifndef TEST_H
#define TEST_H

#include <stdlib.h>
#include <stdint.h>
#include "node.h"
#include "mcslock.h"

/****************************************************************************
*
*   Definitions of constants.
*
****************************************************************************/

#define N_EPOCHS 3
#define NS_PER_S 1000000000
#define CACHESIZE 128           /* Should cover most machines. */
#define MAX_THREADS 65

__attribute__ ((__aligned__(CACHESIZE)))
typedef struct per_thread_t {
    pthread_t thread;
    unsigned long seed;
    size_t ops;

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

    /* Count of logically deleted nodes awaiting reclamation; used by
     * all MR schemes.
     */
    size_t retire_count;

    /* SMR per-thread data:
     *  rlist: retired list
     *  rcount: retired count
     */
    node_t *rlist;
    node_t **plist;

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
} per_thread_t;

typedef enum {
    TEST_NOT_STARTED,
    TEST_RUNNING,
    TEST_OVER,
} test_state_t;

extern uint32_t n_ms;
extern uint32_t n_threads;
extern uint32_t n_elements;
extern uint8_t n_update;
extern uint32_t n_keys;
extern void *global_ds;

extern test_state_t test_state;
extern per_thread_t threads[MAX_THREADS];
extern __thread uint32_t thread_id;

extern node_t *global_freelist;

/****************************************************************************
*
*   Utility macros and functions.
*
****************************************************************************/

#ifdef YIELD
#define cond_yield() sched_yield()
#else
#define cond_yield()
#endif

#define this_thread threads[thread_id]

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
