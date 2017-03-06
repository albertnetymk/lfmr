#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "list.h"
#include "test.h"
#include "util.h"
#include "mr.h"
#include "arch/atomic.h"
#include "random.h"
#include "util.h"
#include <sched.h>
#include <pthread.h>
#include <assert.h>

uint32_t n_ms;
uint32_t n_threads;
uint32_t n_elements;
uint8_t n_update;
uint32_t n_keys;
void *global_ds;

test_state_t test_state;
per_thread_t threads[MAX_THREADS];
__thread uint32_t thread_id;

node_t *global_freelist;

static struct itimerval itimer;

static void *thread_start(void *id)
{
    // setitimer(ITIMER_PROF, &itimer, NULL);

    thread_id = (uint32_t)id;

    /* Seed the random number generator. */
    sRandom((unsigned)((id + 1) + (time(0) << 4)));

    /* Wait for the parent to start the test. */
    while (__atomic_load_n(&test_state, __ATOMIC_ACQUIRE) == TEST_NOT_STARTED) {
        ;
    }

    while (__atomic_load_n(&test_state, __ATOMIC_ACQUIRE) == TEST_RUNNING) {
        testloop_body();
    }

    /* Do any memory reclamation scheme cleanup required. */
    mr_thread_exit();

    return NULL;
}

// #define USE_AFFINITY
static size_t runtest()
{
    // getitimer(ITIMER_PROF, &itimer);
    int ret;
    (void)ret;
    double start, end;                   /* Start and end times for the test. */
    cpu_set_t cpuset, basecpuset;
    (void) cpuset;
    (void) basecpuset;

    /* Make sure the threads don't start prematurely. */
    test_state = TEST_NOT_STARTED;

    /* Re-initialize the memory reclamation scheme. */
    mr_reinitialize();


#ifdef USE_AFFINITY
    /* Get the CPU cpuset. */
    ret = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &basecpuset);
    assert(ret == 0);
#endif

    /* Create our threads. */
    for (uint32_t i = 0; i < n_threads; i++) {
        int ret = pthread_create(
            &threads[i].thread, NULL, thread_start, (void*)(uintptr_t)i);
        assert(ret == 0);
#ifdef USE_AFFINITY
        for (int j = 0; j < CPU_SETSIZE; ++j) {
            if (CPU_ISSET(j, &basecpuset)) {
                CPU_ZERO(&cpuset);
                CPU_SET(j, &cpuset);
                CPU_CLR(j, &basecpuset);
                break;
            }
        }
        /* If we run out of CPUs, distribute threads as evenly as
         * possible across the CPUs. */
        if (CPU_COUNT(&basecpuset) == 0) {
            ret = pthread_getaffinity_np(
                threads[i].thread, sizeof(cpu_set_t), &basecpuset);
            assert(ret == 0);
        }
        ret = pthread_setaffinity_np(
            threads[i].thread, sizeof(cpu_set_t), &cpuset);
        assert(ret == 0);
#endif
    }

    // Start the test and go to sleep for the specified amount of time.
    struct timeval tv;                   /* Lets parent sleep during test. */
    tv.tv_sec = n_ms / 1000;
    tv.tv_usec = (n_ms % 1000) * 1000;
    start = d_gettimeofday();
    test_state = TEST_RUNNING;
    select(0, NULL, NULL, NULL, &tv);

    /* Stop the test; wait for threads to terminate. */
    __atomic_store_n(&test_state, TEST_OVER, __ATOMIC_RELEASE);
    for (uint32_t i = 0; i < n_threads; i++) {
        pthread_join(threads[i].thread, NULL);
    }
    end = d_gettimeofday();

    /* Record the average time per operation for this trial. */
    size_t total_ops = 0;
    for (uint32_t i = 0; i < n_threads; i++) {
        total_ops += threads[i].ops;
    }
    unsigned int co = n_threads <= 64 ? n_threads : 1;
    return (size_t)(co * (end - start) * NS_PER_S) / total_ops;
}

int main(int argc, char **argv)
{
    setup_test(argc, argv);

    size_t ops_per_time = runtest();

    printf("%zu\n", ops_per_time);
}
