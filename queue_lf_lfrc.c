/*
 * Implementation of a lock-free queue based on code in:
 *
 *  M. M. Michael. Hazard Pointers: Safe Memory Reclamation for Lock-Free
 *  Objects. IEEE TPDS (2004) IEEE Transactions on Parallel and Distributed
 *  Systems 15(8), August 2004.
 *
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

#include "queue.h"
#include "lfrc.h"
#include "test.h"
#include "arch/atomic.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include <stdio.h>

struct queue {
    node_t *head  __attribute__ ((__aligned__(CACHESIZE)));
    node_t *tail  __attribute__ ((__aligned__(CACHESIZE)));
};

void queue_init(struct queue **q)
{
    node_t *node = new_node();

    (*q) = (struct queue*)mapmem(sizeof(struct queue));

    if (node == NULL) {
        fprintf(stderr, "queue_init: out of memory\n");
    }

    node->next = NULL;
    lfrc_refcnt_inc(node);     /* head and tail BOTH point to node */
    (*q)->head = node;
    (*q)->tail = node;
}

void queue_destroy(struct queue **q)
{
    free(*q);
}

/* Debugging function. */
long queue_size(struct queue *q)
{
    node_t *cur;
    unsigned long size = 0;

    for (cur = q->head; cur != NULL; cur = cur->next) {
        size++;
    }
    return size;
}

void enqueue(struct queue *q, long key)
{
    node_t *node = new_node();
    node_t *t;
    node_t *next;

    backoff_reset();

    /* Initialize the new node. */
    while (node == NULL) {
        node = new_node();
    }
    lfrc_refcnt_inc(node);     /* for new q->tail */

    node->key = key;
    node->next = NULL;

    /* Can't insert it if it's not pointing to NULL! */
    write_barrier();
    while (1) {
        /* Get the old tail pointer and its successor. */
        t = safe_read(&q->tail);
        next = t->next;

        /* Ensure that t and next are consistent. */
        if (q->tail != t) {
            lfrc_refcnt_dec(t);
            continue;
        }

        /* Help update the tail pointer if needed. */
        if (next != NULL) {
            if (CAS(&q->tail, t, next)) {
                lfrc_refcnt_dec(t);                 /* q->tail */
            }
            lfrc_refcnt_dec(t);             /* t */
            continue;
        }

        /* Attempt to link in the new element. */
        if (CAS(&t->next, NULL, node)) {
            break;
        } else {
            lfrc_refcnt_dec(t);
            backoff_delay();
        }
    }

    /* Swing the tail to the new element. */
    if (CAS(&q->tail, t, node)) {
        lfrc_refcnt_dec(t);         /* for old q->tail */
    }
    lfrc_refcnt_dec(t);     /* for t */
}

long dequeue(struct queue *q)
{
    node_t *h, *t, *next;
    long data;

    backoff_reset();

    while (1) {
        /* Get the old head and tail elements. */
        h = safe_read(&q->head);
        t = safe_read(&q->tail);

        /* Get the head element's successor. */
        next = safe_read(&h->next);
        memory_barrier();
        if (q->head != h) {
            lfrc_refcnt_dec(next);
            lfrc_refcnt_dec(h);
            lfrc_refcnt_dec(t);
            continue;
        }

        /* If the head (dummy) element is the only one, return EMPTY. */
        if (next == NULL) {
            lfrc_refcnt_dec(next);
            lfrc_refcnt_dec(h);
            lfrc_refcnt_dec(t);
            return -1;             /* Empty. */
        }

        /* There are multiple elements. Help update tail if needed. */
        if (h == t) {
            /* enqueuer already set up reference counts */
            if (CAS(&q->tail, t, next)) {
                lfrc_refcnt_dec(t);
            }
            lfrc_refcnt_dec(next);
            lfrc_refcnt_dec(h);
            lfrc_refcnt_dec(t);
            continue;
        }

        /*
         * Save the data of the head's successor. It will become the
         * new dummy node.
         */
        data = next->key;

        /*
         * Attempt to update the head pointer so that it points to the
         * new dummy node.
         */
        if (CAS(&q->head, h, next)) {
            /* Head pointer no longer points to *h */
            lfrc_refcnt_dec(h);
            break;
        } else {
            lfrc_refcnt_dec(next);
            lfrc_refcnt_dec(h);
            lfrc_refcnt_dec(t);
            backoff_delay();
        }
    }

    /* Done with h and t */
    lfrc_refcnt_dec(h);
    lfrc_refcnt_dec(t);

    return data;
}
