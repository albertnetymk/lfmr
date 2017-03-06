#include "dw.h"
#include "test.h"
#include "allocator.h"
#include "util.h"
#include "backoff.h"
#include <assert.h>
#include <stdio.h>

struct list {
    node_t *list_head;
};

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
int find(node_t **head, long key)
{
    node_t **prev, *cur, *next = NULL;

 try_again:
    prev = head;
    cur = *prev;
    for (cur = *prev; cur != NULL; cur = next) {
        if (*prev != cur) {
            goto try_again;
        }
        next = cur->next;

        /* If the bit is marked... */
        if ((unsigned long)next & 1) {
            /* ... update the link and retire the element. */
            if (!CAS(prev, cur, (node_t*)((unsigned long)next - 1))) {
                backoff_delay();
                goto try_again;
            }
            free_node_later(cur);
            next = (node_t*)((unsigned long)next - 1);
        } else {
            long ckey = cur->key;
            if (*prev != cur) {
                goto try_again;
            }
            if (ckey >= key) {
                this_thread.cur = cur;
                this_thread.prev = prev;
                this_thread.next = next;
                return (ckey == key);
            }

            prev = &cur->next;
        }
    }

    this_thread.cur = cur;
    this_thread.prev = prev;
    this_thread.next = next;
    return (0);
}

int insert(struct list *l, long key)
{
    node_t **head = &l->list_head;
    node_t *n = new_node();

    backoff_reset();

    assert(n);
    critical_enter();

    while (1) {
        if (find(head, key)) {
            critical_exit();
            free_node(n);
            return (0);
        }
        n->key = key;
        n->next = this_thread.cur;
        write_barrier();

        if (CAS(this_thread.prev, this_thread.cur, n)) {
            critical_exit();
            return (1);
        } else {
            backoff_delay();
        }
    }
}

int delete(struct list *l, long key)
{
    node_t **head = &l->list_head;

    backoff_reset();

    critical_enter();

    while (1) {
        /* Try to find the key in the list. */
        if (!find(head, key)) {
            critical_exit();
            return (0);
        }

        /* Mark if needed. */
        if (!CAS(&this_thread.cur->next,
                 this_thread.next,
                 (node_t*)((unsigned long)this_thread.next + 1))) {
            backoff_delay();
            continue;             /* Another thread interfered. */
        }

        write_barrier();
        if (CAS(this_thread.prev,
                this_thread.cur, this_thread.next)) {          /* Unlink */
            free_node_later(this_thread.cur);             /* Reclaim */
        }
        /*
         * If we want to revent the possibility of there being an
         * unbounded number of unmarked nodes, add "else _find(head,key)."
         * This is not necessary for correctness.
         */
        critical_exit();
        return (1);
    }
}

int search(struct list *l, long key)
{
    node_t **prev, *cur, *next;

    backoff_reset();

    critical_enter();

 try_again:
    prev = &l->list_head;

    for (cur = *prev; cur != NULL; cur = next) {
        if (*prev != cur) {
            goto try_again;
        }
        next = cur->next;

        /* If the bit is marked... */
        if ((unsigned long)next & 1) {
            /* ... update the link and retire the element. */
            if (!CAS(prev, cur, (node_t*)((unsigned long)next - 1))) {
                backoff_delay();
                goto try_again;
            }
            free_node_later(cur);
            next = (node_t*)((unsigned long)next - 1);
        } else {
            long ckey = cur->key;
            if (*prev != cur) {
                goto try_again;
            }
            if (ckey >= key) {
                critical_exit();
                return (ckey == key);
            }

            prev = &cur->next;
        }
    }

    critical_exit();

    return (0);
}
