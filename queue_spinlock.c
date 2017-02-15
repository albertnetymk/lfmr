/*
 * Simple, dumb, spinlock-based queue. Just like people are likely to use.
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
#include "spinlock.h"
#include "test.h"
#include "arch/atomic.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include <stdio.h>

struct queue {
    node_t *head;
    node_t *tail;
    spinlock_t lock;
};

void queue_init(struct queue **q)
{
    (*q) = (struct queue*)mapmem(sizeof(struct queue));
    (*q)->head = NULL;
    (*q)->tail = NULL;
    (*q)->lock = SPIN_LOCK_UNLOCKED;
}

void queue_destroy(struct queue **q)
{
    free(*q);
}

void enqueue(struct queue *q, long key)
{
    node_t *node = new_node();

    /* Prepare the new node. */
    if (node == NULL) {
        fprintf(stderr, "enqueue: out of memory\n");
        do {} while (1);
    }
    node->next = NULL;
    node->key = key;

    /* Link it in. */
    spin_lock(&q->lock);
    if (q->tail == NULL) {
        q->head = q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
    spin_unlock(&q->lock);
}

long dequeue(struct queue *q)
{
    node_t *node;
    unsigned long data;

    spin_lock(&q->lock);
    if (q->head == NULL) {
        spin_unlock(&q->lock);
        return -1;   /* EMPTY */
    }
    node = q->head;
    if (q->head == q->tail) {
        q->head = q->tail = NULL;
    } else{
        q->head = q->head->next;
    }
    spin_unlock(&q->lock);

    data = node->key;
    free_node(node);
    return data;
}
