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

#include "ebr.h"
#include "test.h"
#include "spinlock.h"
#include <stdio.h>

#define UPDATE_THRESHOLD 100

struct nebr_globals {
	spinlock_t update_lock __attribute__ ((__aligned__ (CACHESIZE)));
	int global_epoch __attribute__ ((__aligned__ (CACHESIZE)));
};

struct nebr_globals *ng __attribute__ ((__aligned__ (CACHESIZE)));

/* Processes a list of callbacks.
 *
 * @list: Pointer to list of node_t's.
 */
void process_callbacks_epoch(node_t **list)
{
	node_t *next;
	node_t *dead;
	unsigned long num = 0;
	
	write_barrier();
	
	for (; (*list) != NULL; (*list) = next) {
		next = (*list)->mr_next;
		free_node(*list);
		num++;
	}

	/* Update our accounting information. */
	this_thread()->rcount -= num;
}

void update_epoch()
{
	int curr_epoch;
	int i;
	int old;
	int myTID = getTID();
	
	if (!spin_trylock(&ng->update_lock)) {
		/* Someone could be preempted while holding the update lock. Give
		 * them the CPU. */
		/*cond_yield();*/
		return;
	}
	
	/* If any CPU hasn't advanced to the current epoch, abort the attempt. */
	curr_epoch = ng->global_epoch;
	for (i = 0; i < tg->nthreads; i++) {
		if (get_thread(i)->in_critical == 1 && 
		    get_thread(i)->epoch != curr_epoch &&
		    i != myTID) {
			spin_unlock(&ng->update_lock);
			/*cond_yield();*/
			return;
		}
	}
	
	/* Update the global epoch. 
	 * 
	 * I wanted to use CAS here, but that would be unsafe due to 
	 * wraparound. */
	ng->global_epoch = (curr_epoch + 1) % N_EPOCHS;
	
	spin_unlock(&ng->update_lock);
	return;
}

void nebr_set_active()
{
	struct per_thread *t = this_thread();
	t->in_critical = 1;
	memory_barrier(); /* Not safe to proceed until our flag is visible. */
}

void nebr_set_inactive()
{
	struct per_thread *t = this_thread();
	memory_barrier(); /* Can't let flag be unset while we're still in
	                   * a lockless op! */
	t->in_critical = 0;
}

void lockless_begin()
{
	struct per_thread *t = this_thread();
	int epoch;
	
 retry:	
	epoch = ng->global_epoch;
	if (t->epoch != epoch) { /* New epoch. */
		/* Process callbacks for old 'incarnation' of this epoch. */
		process_callbacks_epoch(&t->limbo_list[epoch]);
		t->epoch = epoch;
		t->entries_since_update = 0;
	} else if (t->entries_since_update++ == UPDATE_THRESHOLD) {
		t->entries_since_update = 0;
		update_epoch();
		goto retry;
	}
	
	return; /* We're now in a critical section. */
}

void lockless_end()
{
	/* Do nothing. */
}

void mr_init()
{
	int i,j;
	
	ng = (struct nebr_globals *)mapmem(sizeof(struct nebr_globals));

	for (i = 0; i < MAX_THREADS+1; i++) {
		get_thread(i)->epoch = 0;
		get_thread(i)->in_critical = 0;
		get_thread(i)->entries_since_update = 0;
		get_thread(i)->rcount = 0;
		for (j = 0; j < N_EPOCHS; j++)
			get_thread(i)->limbo_list[j] = NULL;
	}
	
	ng->global_epoch = 1;
	ng->update_lock = SPIN_LOCK_UNLOCKED;
}

void mr_thread_exit()
{
	while(this_thread()->rcount > 0)
	{
		update_epoch();
		lockless_begin();
		lockless_end();
		cond_yield();
	}
}

void mr_reinitialize()
{
	int i;
	
	for (i = 0; i < MAX_THREADS+1; i++) {
		get_thread(i)->epoch = 0;
		get_thread(i)->in_critical = 0;
		get_thread(i)->entries_since_update = 0;
		get_thread(i)->rcount = 0;
	}

	ng->global_epoch = 1;
	ng->update_lock = SPIN_LOCK_UNLOCKED;
}

/* Links the node into the per-thread list of pending deletions.
 */
void free_node_later (node_t *q)
{
	struct per_thread *t = this_thread();
	q->mr_next = t->limbo_list[t->epoch];
	t->limbo_list[t->epoch] = q;
	t->rcount++;
}