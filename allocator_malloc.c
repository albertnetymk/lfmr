/* Really dumb allocator: just a wrapper for malloc. */

#include "allocator.h"
#include <stdlib.h>

void init_allocator() {
	/* Do nothing. */
	return;
}

node_t *new_node() {
	return (node_t *)malloc(sizeof(node_t));
}

void free_node(node_t *n)
{
	n->next = 0x11;
	n->mr_next = 0x22;
	free(n);
}

