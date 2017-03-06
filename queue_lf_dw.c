#include "queue.h"
#include "dw.h"
#include "test.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include <stdio.h>
#include <assert.h>

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
    (*q)->head = node;
    (*q)->tail = node;
}

void queue_destroy(struct queue **q)
{
    free(*q);
}

void enqueue(struct queue *q, long key)
{
    node_t *node;
    node_t *t;
    node_t *next;

    backoff_reset();

 retry:
    /* Initialize the new node. */
    critical_enter();
    node = new_node();
    assert(node);

    node->key = key;
    node->next = NULL;

    /* Can't insert it if it's not pointing to NULL! */
    write_barrier();

    while (1) {
        /* Get the old tail pointer. */
        t = q->tail;
        if (q->tail != t) {
            continue;
        }

        /* Help update the tail pointer if needed. */
        next = t->next;
        if (next != NULL) {
            CAS(&q->tail, t, next);
            continue;
        }

        /* Attempt to link in the new element. */
        if (CAS(&t->next, NULL, node)) {
            break;
        } else{
            backoff_delay();
        }
    }

    /* Swing the tail to the new element. */
    CAS(&q->tail, t, node);

    critical_exit();
}

long dequeue(struct queue *q)
{
    node_t *h, *t, *next;
    long data;

    backoff_reset();

    critical_enter();

    while (1) {
        /* Get the old head and tail elements. */
        h = q->head;
        t = q->tail;

        /* Get the head element's successor. */
        next = h->next;
        memory_barrier();
        if (q->head != h) {
            continue;
        }

        /* If the head (dummy) element is the only one, return EMPTY. */
        if (next == NULL) {
            critical_exit();
            return -1;             /* Empty. */
        }

        /* There are multiple elements. Help update tail if needed. */
        if (h == t) {
            CAS(&q->tail, t, next);
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
            break;
        } else{
            backoff_delay();
        }
    }

    /* The old dummy node has been unlinked, so reclaim it. */
    free_node_later(h);

    critical_exit();

    return data;
}
