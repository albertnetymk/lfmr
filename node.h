/*
 * Definition of nodes for our performance tests.
 *
 */
#ifndef NODE_H
#define NODE_H

#include "spinlock.h"

typedef struct node {
    long key;
    struct node *next;
    struct node *mr_next;
    unsigned long refcnt;     /* For lock-free reference counting. */
} node_t;

#endif
