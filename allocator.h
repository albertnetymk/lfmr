#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "node.h"

/* Initialize the allocator. */
void init_allocator();

/* Allocate a new node and return it. */
node_t *new_node();

/* Immediately free a node. */
void free_node(node_t *);

#endif
