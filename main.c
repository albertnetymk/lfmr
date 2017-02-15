/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (c) 2002 IBM Corporation.
 *
 * Copyright (c) 2005 Tom Hart.
 */

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
#include "arch/atomic.h"
#include "random.h"
#include <signal.h>
#include "util.h"
#include <sched.h>

double totals [NUM_TRIALS];

/* Die if we get an unexpected signal.
 */
void sighandler(int a)
{
    fprintf(stderr, "P%d (%d) received signal %d\n",
            getTID(), getpid(), a);
    spin();
}

static void *thread_start()
{
    /* If we segfault, print out an error message and spin forever. */
    signal(SIGSEGV, sighandler);

    /* Seed the random number generator. */
    sRandom((unsigned)((getTID() + 1) + (time(0) << 4)));

    /* Wait for the parent to start the test. */
    while (tg->test_state == TEST_NOT_STARTED) {
        memory_barrier();
    }

    /* Implement in another file to accomodate QSBR, NEBR, etc. */
    test();

    /* Do any memory reclamation scheme cleanup required. */
    mr_thread_exit();

    /* Let the parent know we're done, and exit. */
    (void)atomic_xadd4((atomic_t*)&tg->n_arrived, 1);
    exit(0);
}

int runtest(int iteration)
{
    int i, j;                            /* Iterator variables. */
    struct timeval tv;                   /* Lets parent sleep during test. */
    double start, end;                   /* Start and end times for the test. */
    double total_ops;                    /* Total operations by all threads. */
    int newpid;
    long mask, basemask;
    struct sched_param p;                /* For real-time scheduling. */

    /* Make sure the threads don't start prematurely. */
    tg->test_state = TEST_NOT_STARTED;
    atomic_set4((atomic_t*)&tg->n_arrived, 0);

    /* Re-initialize the memory reclamation scheme. */
    mr_reinitialize();

#ifdef USE_AFFINITY
    /* Get the CPU mask. */
    basemask = 0;
    sched_getaffinity(0, sizeof(basemask), &basemask);
#endif

#ifdef USE_REALTIME
    /* Make sure children are vanilla processes. */
    p.sched_priority = 0;
    sched_setscheduler(0, SCHED_OTHER, &p);
#endif

    /* Create our threads. */
    for (i = 0; i < tg->nthreads; i++) {
        get_thread(i)->op_count = 0;
        newpid = fork();
        if (newpid == 0) {
            g_tid = i;
            thread_start();
            fprintf(stderr, "ERROR: I shouldn't get here!\n");
            spin();
        } else if (newpid == -1) {
            fprintf(stderr, "Cannot fork()\n");
            exit(1);
        } else {
#ifdef USE_AFFINITY
            for (j = 0; j < sizeof(basemask) * 8; j++) {
                if (basemask & (1 << j)) {
                    mask = 1 << j;
                    basemask &= ~(1 << j);
                    break;
                }
            }
            /* If we run out of CPUs, distribute threads as evenly as
             * possible across the CPUs. */
            if (basemask == 0) {
                sched_getaffinity(0, sizeof(basemask), &basemask);
            }
            sched_setaffinity(newpid, sizeof(mask), &mask);
#endif
        }
    }

#ifdef USE_REALTIME
    /* Make sure parent doesn't get starved. */
    p.sched_priority = 99;
    sched_setscheduler(0, SCHED_RR, &p);
#endif

    /* Start the test and go to sleep for the specified amount of time. */
    tv.tv_sec = tg->nmilli / 1000;
    tv.tv_usec = (tg->nmilli % 1000) * 1000;
    start = d_gettimeofday();
    tg->test_state = TEST_RUNNING;
    select(0, NULL, NULL, NULL, &tv);

    /* Stop the test; wait for threads to terminate. */
    tg->test_state = TEST_OVER;
    while (atomic_read4((atomic_t*)&tg->n_arrived) < tg->nthreads) {
        continue;
    }
    end = d_gettimeofday();

    /* Record the average time per operation for this trial. */
    total_ops = 0;
    for (i = 0; i < tg->nthreads; i++) {
        //printf("thread %d did %g ops\n", i, get_thread(i)->op_count);
        total_ops += get_thread(i)->op_count;
    }
    //printf("total ops = %g\n", total_ops);
    totals[iteration] = ((end - start) * NS_PER_S) / total_ops;

    return 0;
}

int main(int argc, char **argv)
{
    int i;                               /* Iterator variable. */
    double av, std, mx, mn;              /* Statistics. */
    int pTID = MAX_THREADS;              /* Parent's TID. */

    /* Initialize our shared memory. */
    tg = (struct test_globals*)mapmem(sizeof(struct test_globals));

    /* Parent needs a space for some freelists for initializations. */
    g_tid = pTID;

    setup_test(argc, argv);
    fflush(0);

    for (i = 0; i < NUM_TRIALS; i++) {
        runtest(i);
    }

    av = avg(totals, NUM_TRIALS);
    std = stdev(totals, NUM_TRIALS);
    mx = max(totals, NUM_TRIALS);
    mn = min(totals, NUM_TRIALS);

    printf("%s: avg = %g  max = %g  min = %g  std = %g\n",
           argv[0], av, mx, mn, std);

    if (tg->out_of_memory > 0) {
        printf("%s: out of memory errors = %lu\n",
               argv[0], tg->out_of_memory);
    }
}
