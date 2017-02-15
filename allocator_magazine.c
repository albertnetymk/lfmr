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
 * Copyright (c) 2005 Tom Hart;
 */

#include "test.h"
#include "node.h"
#include "spinlock.h"
#include "util.h"
#include <stdio.h> /* for stderr() */
#include <string.h> /* for memset() */

#define PT_FREELIST_TARGET 100

/* Push this magazine onto the global "stack" of magazines. */
static void kfree_gbl(node_t *p)
{
	node_t *old;
	do {
		old = tg->global_freelist;
		p->next = old;
		write_barrier();
	} while (!CAS(&tg->global_freelist, old, p));
}

/* Pop a magaine off the global "stack". */
static node_t * kmalloc_gbl()
{
	node_t *p;
	node_t *next;

	do {
		p = tg->global_freelist;
		if (p == NULL) {
			atomic_xadd4(&tg->out_of_memory, 1);
			return NULL; /* Out of memory. */
		}
		next = p->next;
	} while (!CAS(&tg->global_freelist, p, next));
	return p;
}

void free_node(node_t *p)
{
	node_t *q;

	p->next = (node_t *) 0x00300300;
	if (++(this_thread()->freelist_count) > PT_FREELIST_TARGET) {
		if (this_thread()->freelist2 != NULL) {
			q = this_thread()->freelist2;
			this_thread()->freelist2 = NULL;
			kfree_gbl(q);
		}
		this_thread()->freelist2 = this_thread()->freelist;
		this_thread()->freelist = NULL;
		this_thread()->freelist_count = 1;
	}
	p->mr_next = this_thread()->freelist;
	this_thread()->freelist = p;
}

node_t *new_node()
{
	node_t *p;

	/* Make sure freelist has nodes. */
	if (this_thread()->freelist == NULL) {
		if (this_thread()->freelist2 != NULL) {
			this_thread()->freelist = this_thread()->freelist2;
			this_thread()->freelist2 = NULL;
		} else {
			p = kmalloc_gbl();
			if (p == NULL) {
				return (NULL);
			}
			this_thread()->freelist = p;
		}
		this_thread()->freelist_count = PT_FREELIST_TARGET;
	}

	/* We're really out of memory. */
	if (this_thread()->freelist == NULL) return NULL;

	/* Fast path should go straight to this. Pop from freelist. */
	p = this_thread()->freelist;
	this_thread()->freelist = this_thread()->freelist->mr_next;
	this_thread()->freelist_count--;

	/* Clear memory before returning it. */
	memset(p,0,sizeof(node_t));

	return (p);
}

void init_allocator()
{
	int i;
	int j;
	int nelementsfree;
	node_t *shdmem;

	tg->out_of_memory = 0;
	
	nelementsfree = tg->nelements 
		+ (2 * MAX_THREADS * PT_FREELIST_TARGET) 
		+ (MAX_THREADS * 64000);
	
	/* Initialize the global freelist. */
	tg->global_freelist = NULL;

	/* Initialize the per-threads data. "+1" because of the parent.*/
	for (i = 0; i < MAX_THREADS+1; i++) {
		get_thread(i)->freelist  = NULL;
		get_thread(i)->freelist2 = NULL;
		get_thread(i)->freelist_count = 0.;
	}

	/* Allocate a huge chunk of memory. */
	shdmem = (node_t *)mapmem(nelementsfree * sizeof(node_t));
	if (shdmem == NULL) {
		fprintf(stderr, "init_allocator(): Out of memory\n");
		exit(-1);
	}

	/* Initialize the nodes and place them in the freelists. */
	for (i = 0; i < nelementsfree; i++) {
		shdmem[i].key = -1;
		shdmem[i].next = NULL;
		shdmem[i].mr_next = NULL;
		free_node(&shdmem[i]);
	}
}
