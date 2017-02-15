/* 
 * MCS (Mellor-Crummey and Scott) queueing locks.
 *
 * See:
 * "Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors," 
 *  by J. M. Mellor-Crummey and M. L. Scott. ACM Trans. on Computer Systems, 
 *  Feb. 1991.
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
 * Copyright (c) 2005 Thomas E. Hart.
 */

#ifndef __MCSLOCK_H
#define __MCSLOCK_H

#include "arch/atomic.h"

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

typedef struct qnode
{
	int locked;
	/* Hardwire CACHESIZE as 128 to avoid having to #include test.h. */
	struct qnode *next  __attribute__ ((__aligned__ (128)));
} qnode_t;

/* An mcslock is a pointer to the tail of a queue. */
typedef qnode_t* mcslock;

void mcs_lock(mcslock *l, qnode_t *n);
void mcs_unlock(mcslock *l, qnode_t *n);

#endif
