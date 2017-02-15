#include "lfrc.h"
#include "arch/atomic.h"
#include "allocator.h"
#include "test.h"
#include "util.h"
#include <stdio.h> /* for stderr() */
#include "spinlock.h"

#define PT_FREELIST_TARGET 100

/* NOTE:
 *   Regarding the refcnt field, the lowest bit is the "claim" bit, which 
 *   is 1 iff the node is free. The other 31 bits are the reference count.
 *   Since the lowest bit is the claim bit, we always increment and 
 *   decrement the reference count field by 2 so that we leave the lowest
 *   bit alone.
 */

/****************************************************************************
 * Null MR functions.
 ***************************************************************************/

void mr_init() {}
void mr_thread_exit() {}
void mr_reinitialize() {}

/****************************************************************************
 * Utility functions.
 ***************************************************************************/

static unsigned long decrement_and_TAS(unsigned long *p)
{
	unsigned long old, new;
	do {
		old = *p;
		new = old - 2;
		if (new == 0)
			new = 1;
	} while (!CAS(p, old, new));
	/* Return 1 iff reference count is zero AND we're the first 
	 * thread to "claim" this node for reclamation. */
	return (old-new) & 1;
}

static void clear_lowest_bit (unsigned long *p)
{
	unsigned long old, new;
	do {
		old = *p;
		new = old - 1;
	} while (!CAS(p, old, new));
}

/****************************************************************************
 * Customized allocator.
 ***************************************************************************/

/* Initialize the allocator. */
void init_allocator()
{
	int i;
	int j;
	int nelementsfree;
	node_t *shdmem;

	tg->out_of_memory = 0;
	
	nelementsfree = tg->nelements 
		+ (2 * MAX_THREADS * PT_FREELIST_TARGET) 
		+ (MAX_THREADS * 1000);
	
	/* Initialize the global freelist. */
	tg->global_freelist = NULL;

	/* Initialize the per-threads data. */
	/*for (i = 0; i < MAX_THREADS; i++) {
		threads[i].freelist  = NULL;
		threads[i].freelist2 = NULL;
		threads[i].freelist_count = 0.;
		}*/

	/* Allocate a huge chunk of memory. */
	shdmem = (node_t *)mapmem(nelementsfree * sizeof(node_t));
	if (shdmem == NULL) {
		fprintf(stderr, "init_allocator(): Out of memory\n");
		exit(-1);
	}

	/* Initialize the nodes and place them on the freelist. */
	for (i = 0; i < nelementsfree; i++) {
		shdmem[i].key = -1;
		shdmem[i].next = NULL;
		shdmem[i].mr_next = NULL;
		shdmem[i].refcnt = 1; // Initially free
		free_node(&shdmem[i]);
	}
}

/* Allocate a new node and return it. */
node_t *new_node()
{
	node_t *p;
	int x;

	while (1) {
		p = safe_read(&tg->global_freelist);
		if (p == NULL) {
			atomic_xadd4(&tg->out_of_memory, 1);
			return NULL; /* out of memory */
		}
		/* Reference count can be anything here, since multiple threads
		 * could have gotten a reference to the node on the freelist.
		 */
		if (CAS(&tg->global_freelist, p, p->mr_next)) {
			clear_lowest_bit(&p->refcnt);
			return p;
		} else {
			lfrc_refcnt_dec(p);
		}
	}
}

void free_node(node_t *p)
{
	node_t *old;

	p->next = (node_t *)0x300300;
	do {
		old = tg->global_freelist;
		p->mr_next = tg->global_freelist;
		if ((unsigned long)p & 1) { printf("free error\n"); do{}while(1); }
	} while (!CAS(&tg->global_freelist, old, p));
}

/****************************************************************************
 * LFRC functions.
 ***************************************************************************/

/* Reads a pointer to a node and increments that node's reference count. */
node_t *safe_read(node_t **p)
{
	node_t *q;
	node_t *safe;

	while (1) {
		q = *p;
		safe = (node_t *)((long)q & -2);
		if (safe == NULL)
			return q;
		atomic_xadd4(&safe->refcnt, 2);
		memory_barrier();
		/* If the pointer hasn't changed its value.... */
		if (q == *p) { 
			/*if (safe->refcnt & 1) {
				printf("safe_read error: q->refcnt = %lu\n", 
					  safe->refcnt);
				do{}while(1);
				}*/
			return q;
		} else
			lfrc_refcnt_dec(safe);
	}
}

/* Decrement reference count and see if the node can be safely freed. */
void lfrc_refcnt_dec(node_t *p)
{
	/* It's possible to acquire a reference to a marked node, and then
	 * want to release it. This stops us from getting a bus error.
	 */
	p = (node_t *)((long)p & (-2));

	if (p == NULL)
		return;

	/* 0 => no more references to this node, but someone else is already 
	 * reclaiming it. */
	memory_barrier();
	if (decrement_and_TAS(&p->refcnt) == 0)
		return;

	/* The lock-free list may have marked p's next pointer. If we're here,
	 * it's safe to unmark it (no node can get a reference to it), and 
	 * also necessary (or else we'll have a bus error when we try to
	 * access non-aligned memory).
	 */
	p->next = (node_t *)((long)p->next & (-2));
	lfrc_refcnt_dec(p->next);
	free_node(p);
}

/* Increment reference count. */
void lfrc_refcnt_inc(node_t *p)
{
	if (p == NULL)
		return;
	atomic_xadd4(&p->refcnt, 2);
	memory_barrier();
}
