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
 * Copyright (c) Thomas E. Hart.
 */

#include "smr.h"
#include "test.h"
#include "arch/atomic.h"
#include "allocator.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void mr_init()
{
    int i;

    /* Allocate HP array. Over-allocate since the parent has pid 32. */
    HP = (hazard_pointer*)mapmem(sizeof(hazard_pointer) * K * (MAX_THREADS + 1));
    if (HP == NULL) {
        fprintf(stderr, "SMR mr_init: out of memory\n");
        exit(-1);
    }

    /* Initialize rlists. */
    for (i = 0; i < MAX_THREADS + 1; i++) {
        threads[i].rlist = NULL;
        threads[i].retire_count = 0;
        threads[i].plist = (node_t**)
                           malloc(sizeof(node_t *) * K * n_threads);
    }

    /* Initialize the hazard pointers. */
    for (i = 0; i < K * (MAX_THREADS + 1); i++) {
        HP[i].p = NULL;
    }
}

void mr_thread_exit()
{
    unsigned long myTID = thread_id;
    uint32_t i;

    for (i = 0; i < K; i++) {
        HP[K * myTID + i].p = NULL;
    }

    while (this_thread.retire_count > 0) {
        scan();
        cond_yield();
    }
}

void mr_reinitialize()
{
}

/*
 * Comparison function for qsort.
 *
 * We just need any total order, so we'll use the arithmetic order
 * of pointers on the machine.
 *
 * Output (see "man qsort"):
 *  < 0 : a < b
 *    0 : a == b
 *  > 0 : a > b
 */
int compare(const void *a, const void *b)
{
    return (int)(*(node_t**)a - *(node_t**)b);
}

/* Debugging function. Leave it around. */
inline node_t *ssearch(node_t **list, int size, node_t *key)
{
    int i;
    for (i = 0; i < size; i++) {
        if (list[i] == key) {
            return list[i];
        }
    }
    return NULL;
}

void scan()
{
    /* Iteratation variables. */
    node_t *cur;
    uint32_t i;

    /* List of SMR callbacks. */
    node_t *tmplist;

    /* List of hazard pointers, and its size. */
    node_t **plist = this_thread.plist;
    unsigned long psize;

    /*
     * Make sure that the most recent node to be deleted has been unlinked
     * in all processors' views.
     *
     * Good:
     *   A -> B -> C ---> A -> C ---> A -> C
     *                    B -> C      B -> POISON
     *
     * Illegal:
     *   A -> B -> C ---> A -> B      ---> A -> C
     *                    B -> POISON      B -> POISON
     */
    write_barrier();

    /* Stage 1: Scan HP list and insert non-null values in plist. */
    psize = 0;
    for (i = 0; i < H; i++) {
        if (HP[i].p != NULL) {
            plist[psize++] = HP[i].p;
        }
    }

    /* Stage 2: Sort the plist. */
    qsort(plist, psize, sizeof(node_t *), compare);

    /* Stage 3: Free non-harzardous nodes. */
    tmplist = this_thread.rlist;
    this_thread.rlist = NULL;
    this_thread.retire_count = 0;
    while (tmplist != NULL) {
        /* Pop cur off top of tmplist. */
        cur = tmplist;
        tmplist = tmplist->mr_next;

        if (bsearch(&cur, plist, psize, sizeof(node_t *), compare)) {
            cur->mr_next = this_thread.rlist;
            this_thread.rlist = cur;
            this_thread.retire_count++;
        } else {
            free_node(cur);
        }
    }
}

void free_node_later(node_t *n)
{
    n->mr_next = this_thread.rlist;
    this_thread.rlist = n;
    this_thread.retire_count++;

    if (this_thread.retire_count >= R) {
        scan();
    }
}
