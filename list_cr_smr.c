/*
 * Concurrently-readable linked list using SMR.
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
#include "smr.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include <stdio.h>

struct list {
    node_t *list_head;
    spinlock_t list_lock;
};

void list_init(struct list **l)
{
    *l = (struct list*)mapmem(sizeof(struct list));
    (*l)->list_head = (void*)0;
    (*l)->list_lock = SPIN_LOCK_UNLOCKED;
}

void list_destroy(struct list **l)
{
    free(*l);
}

#define clean_pointer(p)      (node_t*)((unsigned long)((p)) & (-2))

int search(struct list *l, long key)
{
    node_t **prev, *cur, *next;
    int base = getTID() * K;
    int off = 0;

 try_again:
    prev = &l->list_head;

    for (cur = *prev; cur != NULL; cur = clean_pointer(next)) {
        /* Protect cur with a hazard pointer. */
        HP[base + off].p = cur;
        memory_barrier();
        if (*prev != cur) {
            goto try_again;
        }

        next = cur->next;

        if (cur->key >= key) {
            return (cur->key == key);
        }

        prev = &cur->next;
        off = !off;
    }
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
    node_t *next;

    spin_lock(&l->list_lock);

    if (!find(&l->list_head, key, myTID)) {
        spin_unlock(&l->list_lock);
        return 0;
    }

    next = this_thread()->cur->next;

    /* Mark the pointer to facilitate hazard pointer validation. This
     * is necessary, since if we don't mark this bit, cur could be
     * removed, and then next could be removed (and deleted), and
     * a reader wouldn't be able to detect this, since it'd be
     * prev's next pointer, and not cur's, that had changed.
     *
     * No write barrier is necessary -- scan() has one. */
    this_thread()->cur->next =
        (node_t*)((unsigned long)this_thread()->cur->next + 1);

    /* Logically delete cur. */
    *this_thread()->prev = next;

    free_node_later(this_thread()->cur);

    spin_unlock(&l->list_lock);
    return 1;
}

int insert(struct list *l, long key)
{
    int myTID = getTID();
    node_t *n = new_node();

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
