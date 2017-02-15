/*
 * Linked list protected by a single lock.
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
 * Copyright (c) 2002 IBM Corporation.
 * Copyright (c) 2005 Tom Hart.
 */

#include <stdio.h>
#include <string.h>
#include "spinlock.h"
#include "smr.h"
#include "list.h"
#include "node.h"
#include "allocator.h"
#include "util.h"
#include <stdio.h>

struct list {
    spinlock_t list_lock;
    node_t *list_head;
};

void list_init(struct list **l)
{
    *l = (struct list*)mapmem(sizeof(struct list));
    (*l)->list_lock = SPIN_LOCK_UNLOCKED;
    (*l)->list_head = NULL;
}

void list_destroy(struct list **l)
{
    free(*l);
}

/* Debugging function. */
void show(struct list *l)
{
    node_t *cur;

    for (cur = l->list_head; cur != NULL; cur = cur->next) {
        printf("{%d} ", cur->key);
    }
    printf("\n");
}

int search(struct list *l, long key)
{
    node_t *cur;

    spin_lock(&l->list_lock);

    for (cur = l->list_head; cur != NULL; cur = cur->next) {
        if (cur->key >= key) {
            spin_unlock(&l->list_lock);
            return (cur->key == key);
        }
    }

    spin_unlock(&l->list_lock);
    return 0;
}

int delete(struct list *l, long key)
{
    node_t *cur;
    node_t **prev;

    spin_lock(&l->list_lock);

    prev = &l->list_head;

    for (cur = *prev; cur != NULL; cur = cur->next) {
        if (cur->key == key) {
            *prev = cur->next;
            free_node(cur);
            spin_unlock(&l->list_lock);
            return 1;
        }
        prev = &cur->next;
    }

    spin_unlock(&l->list_lock);
    return 0;
}

int insert(struct list *l, long key)
{
    node_t *cur;
    node_t **prev;
    node_t *newnode;

    spin_lock(&l->list_lock);

    /* Find cur, prev s. th. *prev=cur, and (cur=NULL or cur->key>=key). */
    prev = &l->list_head;
    for (cur = *prev;
         cur != NULL && cur->key < key;
         cur = cur->next, prev = &(*prev)->next) {
    }

    /* Abort if the key is already in the list. */
    if (cur != NULL) {
        if (cur->key == key) {
            spin_unlock(&l->list_lock);
            return 0;
        }
    }

    /* Insert the new key. */
    newnode = new_node();
    newnode->key = key;
    newnode->next = cur;
    *prev = newnode;

    spin_unlock(&l->list_lock);
    return 1;
}
