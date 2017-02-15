/*
 * Concurrently-readable linked list using EBR.
 *
 * Parts copied from:
 *   P. E. McKenney. "RCU vs. Locking Performance on Different CPUs". 
 *   linux.conf.au, Adelaide, Australia: January 2004. 
 *
 *   URL: http://www.rdrop.com/users/paulmck/rclock/lockperf.2004.01.17a.pdf
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
 * Copyright (c) IBM Corporation.
 * Copyright (c) Thomas E. Hart.
 */

#include <stdio.h>
#include "spinlock.h"
#include "allocator.h"
#include "util.h"
#include "ebr.h"
#include "test.h"
#include <stdio.h>

struct list {
	node_t *list_head;
	spinlock_t list_lock;
};

void list_init(struct list **l) {
	*l = (struct list *)mapmem(sizeof(struct list));
	(*l)->list_head = (void *)0;
	(*l)->list_lock = SPIN_LOCK_UNLOCKED;
}

void list_destroy(struct list **l) {
	free(*l);
}

int search (struct list *l, long key)
{
  	node_t *cur;

	critical_enter();
		
	for (cur = l->list_head; cur != NULL; cur = cur->next) {
		if (cur->key >= key) {
			critical_exit();
			return (cur->key == key);
		}
	}
	
	critical_exit();
	return (0);
}

/* NB: This a a helper function called with the list lock held. */
int find(node_t **head, long key, unsigned long myTID)
{
	node_t **prev, *cur;
	
	prev = head;
	for (cur = *prev; cur != NULL; cur = cur->next) {
		if (cur->key >= key) {
			this_thread()->prev = prev;
			this_thread()->cur = cur;
			return (cur->key == key);
		}
		prev = &cur->next;
	}

	/* Didn't find the key. */
	this_thread()->prev = prev;
	this_thread()->cur = cur;
	return 0;
}

int delete(struct list *l, long key)
{
	int myTID = getTID();

	spin_lock(&l->list_lock);
	critical_enter();

	if (!find(&l->list_head, key, myTID)) {
		critical_exit();
		spin_unlock(&l->list_lock);
		return 0;
	}

	/* Logically delete cur. */
	*this_thread()->prev = this_thread()->cur->next;

	/* Deferred deletion. */
	free_node_later(this_thread()->cur);

	critical_exit();
	spin_unlock(&l->list_lock);
	return 1;
}

int insert(struct list *l, long key)
{
	int myTID = getTID();
	node_t *n = new_node();

	while (n == NULL) {
		critical_enter();
		critical_exit();
		update_epoch();
		n = new_node();
		if (n == NULL) cond_yield();
	}

	spin_lock(&l->list_lock);

	if (find(&l->list_head, key, myTID)) {
		spin_unlock(&l->list_lock);
		free_node(n);
		return (0);
	}
	
	n->key = key;
	n->next = this_thread()->cur;
	write_barrier();
	*this_thread()->prev = n;
	
	spin_unlock(&l->list_lock);
	return (1);
}
