/*
 * Implementation of a lock-free list-based set based on Michael's
 * modification of Harris' original algorithm using hazard pointers.
 *
 * Follows the pseudocode given in :
 *  M. M. Michael. Hazard Pointers: Safe Memory Reclamation for Lock-Free
 *  Objects. IEEE TPDS (2004) IEEE Transactions on Parallel and Distributed
 *  Systems 15(8), August 2004.
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
 * Copyright (c) Thomas E. Hart.
 */

#include "lfrc.h"
#include "test.h"
#include "arch/atomic.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include <stdio.h>

struct list {
    node_t *list_head;
};

/* We need to maintain pointers to "prev" and "cur" for reference counting
 * purposes, but "prev" is a node**, not a node*, which makes things
 * difficult, and we need these to be visible from multiple functions.
 *
 * Solve this problem by having an array of refs here, similar to the
 * hazard pointer array.
 *
 * Well, now that we're using fork() instead of pthreads, just make it an
 * array of size 3. ;-)
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

void list_init(struct list **l)
{
    *l = (struct list*)mapmem(sizeof(struct list));
    (*l)->list_head = NULL;
}

void list_destroy(struct list **l)
{
    free(*l);
}

/*
 * Takes advantage of pointers always lying on 4-byte boundaries
 * since words are 32-bits on x86. Hence we can mark the low-order
 * two bytes of a pointer. We only mark the low-order one.
 *
 * Postcondition:
 *   1. If list is empty, prev=head, cur=NULL, return false.
 *   2. If we find a q s. th. q->key >= key (q has the *smallest* key <= key),
 *      then prev = &cur, cur = q, an we return (q->key == key).
 *   3. If the key isn't in the list, *prev=(last node), cur=NULL, next=null,
 *      return false.
 *
 * NOTE: It is this postcondition, as it applies to prev, that requires us to
 *       use two levels of indirection for the head: we need prev to point to
 *       the right thing in the calling function, not a local head variable.
 */
int find(node_t **head, long key, unsigned long myTID)
{
    node_t **tmp;
    node_t **prev, *cur, *next;
    node_t **ref0, **ref1, **ref2;

    ref0 = &refs[0];
    ref1 = &refs[1];
    ref2 = &refs[2];

 try_again:
    *ref0 = *ref1 = *ref2 = NULL;
    prev = head;
    cur = safe_read(prev);
    *ref2 = cur;
    for (; cur != NULL; cur = next) {
        /* Done with the old *ref0. */
        next = safe_read(&cur->next);
        lfrc_refcnt_dec(*ref0);
        *ref0 = next;

        /* If the bit is marked... */
        if ((unsigned long)next & 1) {
            /* ... update the link and retire the element. */
            if (!CAS(prev, cur, (node_t*)((unsigned long)next - 1))) {
                release_temp_refs(myTID);
                backoff_delay();
                goto try_again;
            } else {
                lfrc_refcnt_dec(cur);
            }
            next = (node_t*)((unsigned long)next - 1);
        } else {
            if (*prev != cur) {
                release_temp_refs(myTID);
                goto try_again;
            }
            if (cur->key >= key) {
                this_thread()->cur = cur;
                this_thread()->prev = prev;
                this_thread()->next = next;
                return (cur->key == key);
            }

            prev = &cur->next;

            /* Pull the same trick we use with the hazard pointers. */
            tmp = ref1;
            ref1 = ref2;
            ref2 = ref0;
            ref0 = tmp;
        }
    }

    this_thread()->cur = cur;
    this_thread()->prev = prev;
    this_thread()->next = next;
    return (0);
}

int insert(struct list *l, long key)
{
    node_t **head = &l->list_head;
    node_t *n = new_node();
    unsigned long myTID = getTID();
    node_t *cur;

    backoff_reset();

    while (1) {
        if (find(head, key, myTID)) {
            /* Must set n->next to NULL; otherwise, when we decrement its
             * reference count, we'll also erroneously decrement the
             * reference count of whatever it's next pointer points to.
             */
            n->next = NULL;
            lfrc_refcnt_dec(n);
            release_temp_refs(myTID);
            return (0);
        }

        n->key = key;
        n->next = this_thread()->cur;
        write_barrier();

        if (CAS(this_thread()->prev, this_thread()->cur, n)) {
            /* cur still has the same number of references to it */
            release_temp_refs(myTID);
            return (1);
        } else {
            release_temp_refs(myTID);
            backoff_delay();
        }
    }
}

int delete(struct list *l, long key)
{
    node_t **head = &l->list_head;
    unsigned long myTID = getTID();
    node_t *cur;

    backoff_reset();

    while (1) {
        /* Try to find the key in the list. */
        if (!find(head, key, myTID)) {
            release_temp_refs(myTID);
            return (0);
        }

        /* We'll be setting a new reference to next if we're successful,
         * but also getting rid of cur's reference to it. We must
         * increment the reference count for the new reference, since the
         * old one will be automatically taken away.
         *
         * Increment the reference HERE so that helpers don't have to
         * worry about it. */
        lfrc_refcnt_inc(this_thread()->next);

        /* Mark if needed. */
        if (!CAS(&this_thread()->cur->next,
                 this_thread()->next,
                 (node_t*)((unsigned long)this_thread()->next + 1))) {
            release_temp_refs(myTID);
            backoff_delay();
            continue;             /* Another thread interfered. */
        }

        write_barrier();

        if (CAS(this_thread()->prev,
                this_thread()->cur, this_thread()->next)) {            /* Unlink */
            lfrc_refcnt_dec(this_thread()->cur);
        }

        release_temp_refs(myTID);
        return (1);
    }
}

int search(struct list *l, long key)
{
    node_t **tmp;
    node_t **prev, *cur, *next;
    node_t **ref0, **ref1, **ref2;
    unsigned long myTID = getTID();
    int retval;

    backoff_reset();

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

        /* If the bit is marked... */
        if ((unsigned long)next & 1) {
            /* ... update the link and retire the element. */
            if (!CAS(prev, cur, (node_t*)((unsigned long)next - 1))) {
                release_temp_refs(myTID);
                backoff_delay();
                goto try_again;
            } else {
                lfrc_refcnt_dec(cur);
            }
            next = (node_t*)((unsigned long)next - 1);
        } else {
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
    }

    release_temp_refs(myTID);
    return (0);
}
