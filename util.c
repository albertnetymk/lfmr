#include "util.h"
#include "mcslock.h"
#include <math.h>
#include <stdlib.h>
#include <sys/mman.h>

double d_gettimeofday()
{
    int retval;
    struct timeval tv;

    retval = gettimeofday(&tv, NULL);
    if (retval != 0) {
        perror("gettimeofday");
        exit(-1);
    }
    return (tv.tv_sec + ((double)tv.tv_usec) / 1000000.);
}

double avg(double *data, int samples)
{
    int i;
    double sum = 0;

    for (i = 0; i < samples; i++) {
        sum += data[i];
    }

    return sum / (double)samples;
}

double stdev(double *data, int samples)
{
    int i;
    double sumsq = 0;
    double av;

    for (i = 0; i < samples; i++) {
        sumsq += data[i] * data[i];
    }
    av = avg(data, samples);

    return sqrt(sumsq / samples - av * av);
}

double max(double *data, int samples)
{
    int i;
    double m = data[0];

    for (i = 1; i < samples; i++) {
        if (data[i] > m) {
            m = data[i];
        }
    }

    return m;
}

double min(double *data, int samples)
{
    int i;
    double m = data[0];

    for (i = 1; i < samples; i++) {
        if (data[i] < m) {
            m = data[i];
        }
    }

    return m;
}

/*
 * In theory, we shouldn't need this, since with pthreads malloc() should
 * suffice. However, malloc() proved finicky and addresses were getting
 * corrupted. mmap() doesn't seem to suffer from this problem, so use
 * it we shall.
 */
void *mapmem(size_t length)
{
    void *p;

    p = mmap(NULL, length,
             PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED,
             -1, (off_t)0);
    if (p == (void*)-1) {
        perror("mmap");
        exit(-1);
    }
    return p;
}

/* MCS locking operations. Put them here for the sake of convenience. */

void mcs_lock(mcslock *l, qnode_t *n)
{
    qnode_t *pred;

    /* Initialize my data. */
    n->next = NULL;
    n->locked = FALSE;

    /* Enqueue myself. */
    pred = (qnode_t*)atomic_xchg4((atomic_t*)l, (unsigned long)n);
    if (pred != NULL) {
        n->locked = TRUE;
        pred->next = n;
    }

    /* Spin. */
    while (n->locked) {
        memory_barrier();
    }
    spin_lock_barrier();
}

void mcs_unlock(mcslock *l, qnode_t *n)
{
    spin_unlock_barrier();

    /* Fast exit if we're alone. */
    if (n->next == NULL) {
        if (CAS(l, n, NULL)) {
            return;
        }
    }

    /* Another processor executed a fetch-and-store (atomic_xchg4). Wait for
     * it to set its locked and next fields. */
    while (n->next == NULL) {
        memory_barrier();
    }

    /* Signal the next waiting processor to go. */
    n->next->locked = FALSE;
}
