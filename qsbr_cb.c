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

#include "qsbr.h"
#include "mr.h"
#include "test.h"
#include <stdio.h>
#include "arch/atomic.h"

#define NOT_ENTERED 0
#define ENTERED 1
#define DONE 2

typedef unsigned long __attribute__ ((__aligned__ (CACHESIZE))) aligned_ulong;

struct qsbr_globals {
	aligned_ulong this_round;
	aligned_ulong next_round;
};

struct qsbr_globals *qg;

void mr_init()
{
	int i;
	
	qg = (struct qsbr_globals *)mapmem(sizeof(struct qsbr_globals));

	for (i = 0; i < MAX_THREADS+1; i++) {
		get_thread(i)->flag = NOT_ENTERED;
		get_thread(i)->qsbr_currlist = NULL;
		get_thread(i)->qsbr_midlist = NULL;
		get_thread(i)->qsbr_nxtlist = NULL;
		get_thread(i)->rcount = 0;
	}
	
	qg->this_round = tg->nthreads;
	qg->next_round = 0;
}

void mr_thread_exit()
{
	int i;

	/* Don't exit without processing our callbacks.*/
	do {
		quiescent_state(FUZZY);
		cond_yield();
	} while(this_thread()->rcount > 0);

	if (this_thread()->flag == ENTERED) {
		/* I've incremented next_round, but I want out.
		 * Only safe thing to do is to wait until 
		 * my flag is DONE, and then do a FAS. */
		while(this_thread()->flag != DONE) {
			memory_barrier();
			cond_yield();
		}
	}

	/* Now we know that I've been registered for a round, 
	 * but I'm leaving. */
	if (atomic_xadd4(&qg->this_round, (unsigned long)(-1)) == 1){
		/* We were the last thread. */
		qg->this_round = qg->next_round;
		qg->next_round = 0;
		write_barrier();
		for (i = 0; i < tg->nthreads; i++)
			get_thread(i)->flag = DONE;
	}
}

void mr_reinitialize()
{
	int i;

	for (i = 0; i < MAX_THREADS+1; i++) {
		get_thread(i)->flag = NOT_ENTERED;
	}
	
	qg->this_round = tg->nthreads;
	qg->next_round = 0;
}

/* Debugging function. */
void show_list(node_t **list, unsigned long n)
{
	node_t *cur = *list;
	int i;

	while(cur != NULL) {
	//for (i = 0; i < n; i++) {
		printf("{0x%0x} \n", cur);
		if (cur->mr_next == NULL) break;
		cur = cur->mr_next;
	}
	printf("\n");
}

/* Debugging function. */
unsigned long list_size(node_t **list)
{
	unsigned long size = 0;
	node_t *cur, *old;
	
	old = NULL;
	for (cur = *list; cur != NULL; cur = cur->mr_next) {
		/*if(size == threads[getTID()].nc+1) {
			printf("[%lu] cur = 0x%0x\told = 0x%0x\n",
				getTID(), cur, old);
			//getchar();
		}*/
		size++;
		old = cur;
	}

	return size;
}

/* Processes a list of callbacks.
 *
 * @list: Pointer to list of node_t's.
 */
void process_callbacks(node_t **list)
{
	node_t *next;
	unsigned long num = 0;
	
	write_barrier();

	for (; (*list) != NULL; (*list) = next) {
		next = (*list)->mr_next;
		(*list)->mr_next = NULL;
		free_node(*list);
		num++;
	}

	/* Update our accounting information. */
	//if (num > 0) printf("thread %lu processed %lu nodes\n", getTID(), num);
	this_thread()->rcount -= num;
}

/* 
 * Informs other threads that this thread has passed through a quiescent 
 * state.
 * If all threads have passed through a quiescent state since the last time
 * this thread processed it's callbacks, proceed to process pending callbacks.
 */
void quiescent_state (int blocking)
{
	int i;
	int me = getTID();
	unsigned long sz;
	qnode_t *nd = &this_thread()->mynode;

	/* 
	 * Scheme to reduce cache-miss complexity: use a fuzzy barrier with
	 * a centralized bitmask.
	 *
	 * For a really huge (more than 32, definitely at 512) number of cpus, 
	 * it would be worth investing in a combining tree.
	 *
	 * Notes on threads[N].flag:
	 *   - only thread N sets threads[n].flag to ENTERED or NOT_ENTERED
	 *   - the thread who sees the "full" qsbr_bitmask sets all 
	 *     threads[i].flag to DONE for all other threads i; since 
	 *     qsbr_bitmask is seen by one thread (the lock holder), this 
	 *     updater thread is unique on any barrier iteration
	 */
retry:
	switch(this_thread()->flag) {
		case NOT_ENTERED: /* Try to enter the barrier. */
			/* Move qsbr_nxtlist to qsbr_midlist. */
			this_thread()->qsbr_midlist = this_thread()->qsbr_nxtlist;
			this_thread()->qsbr_nxtlist = NULL;

			atomic_xadd4(&qg->next_round, 1);
			this_thread()->flag = ENTERED;
			write_barrier();
			if (atomic_xadd4(&qg->this_round, (unsigned long)(-1)) == 1){
				/* We were the last thread. */
				qg->this_round = qg->next_round;
				qg->next_round = 0;
				write_barrier();
				for (i = 0; i < tg->nthreads; i++)
					get_thread(i)->flag = DONE;
				this_thread()->flag = NOT_ENTERED;
				break; /* Process my callbacks. */
			}
			if (blocking) {
				cond_yield();
				goto retry; /* Spin. */
			} else
				goto end; /* Abort the attempt. */
			/* Shouldn't be hit, but stops compiler from complaining. */
			break;
		case DONE:
			this_thread()->flag = NOT_ENTERED;
			break; /* Process my callbacks. */
		case ENTERED:
			/* Tried putting the yielding here. It helped with preemption, 
			 * but introduced overhead in the common case. Better only
			 * to yield if we're out of memory. 
			 */
			if (blocking) {
				/* Let other threads catch up. */
				cond_yield();
				goto retry; /* Spin. */
			} else
				goto end; /* Abort the attempt. */
			/* Shouldn't be hit, but stops compiler from complaining. */
			break; 
		default:
			abort();
	}

	/* Process the callbacks. */
	//printf("[%lu] Processing qsbr callbacks\n", me);
	process_callbacks(&this_thread()->qsbr_currlist);
	//printf("[%lu] processed\n", me);

	/* Move qsbr_midlist to qsbr_currlist. */
	this_thread()->qsbr_currlist = this_thread()->qsbr_midlist;
	this_thread()->qsbr_midlist = NULL;
end:
	return;
}

/* Links the node into the per-thread list of pending deletions.
 */
void free_node_later (node_t *q)
{
	q->mr_next = this_thread()->qsbr_nxtlist;
	this_thread()->qsbr_nxtlist = q;
	this_thread()->rcount++;
}
