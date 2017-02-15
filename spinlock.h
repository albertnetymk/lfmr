/*
 * Spinlock operations.  These were hand-coded specially for this
 * effort without reference to GPL code.
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
 * Copyright (c) 2003 IBM Corporation.
 */

#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#include "arch/atomic.h"
#include <stdlib.h>
#include "backoff.h"

typedef atomic_t spinlock_t;

#define SPIN_LOCK_UNLOCKED 0

/*
 * Acquire a spinlock.
 */

static inline void
spin_lock(spinlock_t *slp)
{
	unsigned long result;

	backoff_reset();

	for (;;) {
		result = atomic_cmpxchg4(slp, 0, 1);
		if (result == 0) {
			spin_lock_barrier();
			return;
		}
		while (*slp != 0) {
			backoff_delay();
			memory_barrier();
		}
	}
}

/*
 * Conditionally acquire a spinlock.
 */

static inline int
spin_trylock(spinlock_t *slp)
{
	unsigned long result;

	result = atomic_cmpxchg4(slp, 0, 1);
	spin_lock_barrier();
	return (!result);
}

/*
 * Release a spinlock.
 */

static inline void
spin_unlock(spinlock_t *slp)
{
	unsigned long result;

	spin_unlock_barrier();
	if ((result = atomic_xchg4(slp, SPIN_LOCK_UNLOCKED)) != 1) {
		abort();
	}
}

#endif /* #ifndef __SPINLOCK_H */
