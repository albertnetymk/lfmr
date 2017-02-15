/*
 * Concurrently-readable linked list using LFRC.
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
#include <pthread.h>
#include "spinlock.h"
#include "lfrc.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include "test.h"
#include <stdio.h>

/* We need to maintain pointers to "prev" and "cur" for reference counting
 * purposes, but "prev" is a node**, not a node*, which makes things
 * difficult, and we need these to be visible from multiple functions.
 *
 * Solve this problem by having an array of refs here, similar to the
 * hazard pointer array.
 *
 * Again, array of size 3 with fork(). :p
 */
node_t *refs[3];

/* Helper function. */
static inline void release_temp_refs(unsigned long myTID)
{
    lfrc_refcnt_dec(refs[0]);
    lfrc_refcnt_dec(refs[1]);
    lfrc_refcnt_dec(refs[2]);
    refs[0] = refs[1] = refs[2] = NULL;
}

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

int search(struct list *l, long key)
{
    node_t **tmp;
    node_t **prev, *cur, *next;
    node_t **ref0, **ref1, **ref2;
    unsigned long myTID = getTID();
    int retval;

    ref0 = &refs[0];
    ref1 = &refs[1];
    ref2 = &refs[2];

 try_again:
    *ref0 = *ref1 = *ref2 = NULL;
    prev = &l->list_head;
    cur = safe_read(prev);
    *ref2 = cur;
    for (; cur != NULL; cur = next) {
        /* Done with the old *ref0. */
        next = safe_read(&cur->next);
        lfrc_refcnt_dec(*ref0);
        *ref0 = next;

        if (*prev != cur) {
            release_temp_refs(myTID);
            goto try_again;
        }
        if (cur->key >= key) {
            retval = (cur->key == key);
            release_temp_refs(myTID);
            return retval;
        }

        prev = &cur->next;

        /* Pull the same trick we use with the hazard pointers. */
        tmp = ref1;
        ref1 = ref2;
        ref2 = ref0;
        ref0 = tmp;
    }
    release_temp_refs(myTID);
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

    if (!find(&l->list_head, key, myTID)) {
        spin_unlock(&l->list_lock);
        return 0;
    }

    /* Logically delete cur. */
    /* We'll be setting a new reference to cur->next if we're successful,
     * but also getting rid of cur's reference to it. We must
     * increment the reference count for the new reference, since the
     * old one will be automatically taken away.
     */
    lfrc_refcnt_inc(this_thread()->cur->next);
    *this_thread()->prev = this_thread()->cur->next;
    lfrc_refcnt_dec(this_thread()->cur);

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
        n->next = NULL;
        lfrc_refcnt_dec(n);
        return (0);
    }

    n->key = key;
    n->next = this_thread()->cur;
    write_barrier();
    *this_thread()->prev = n;

    spin_unlock(&l->list_lock);
    return (1);
}
