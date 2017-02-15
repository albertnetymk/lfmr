/*
 * Atomic operations for x86 CPUs.  Many operations will not work
 * on 80386.  Pentium is the only guarantee.
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
 */

#ifndef _ATOMIC_H
#define _ATOMIC_H
#define _ATOMIC_H_X86
/*
 * Wrapper for atomic_cmpxchg_ptr that provides more suitable semantics for
 * use in most published CAS-based lock-free algorithms.
 */
#define CAS(ptr, oldval, newval) (atomic_cmpxchg_ptr((ptr), (oldval), (newval)) == (oldval))
#define atomic_cmpxchg_ptr(p, o, n) \
	(typeof(*(p)))atomic_cmpxchg4((atomic_t *)p, (atomic_t)o, (atomic_t)n)
#define atomic_read_ptr(p) \
	(typeof(*(p)))atomic_read4((atomic_t *)p)

typedef unsigned long atomic_t;

static inline unsigned long atomic_xchg4(volatile atomic_t *ptr,
					 unsigned long value);

/*
 * Atomically compare and exchange the value pointed to by ptr
 * with newval, but only if the pointed-to value is equal to
 * oldval.  In either case, return the value originally pointed
 * to by ptr.
 */

static inline unsigned long
atomic_cmpxchg4(volatile atomic_t *ptr,
	        unsigned long oldval,
		unsigned long newval)
{
	unsigned long retval;

	asm("# atomic_cmpxchg4\n"
	    "lock; cmpxchgl %4,(%2)\n"
	    "# end atomic_cmpxchg4"
	    : "=a" (retval), "=m" (*ptr)
	    : "r" (ptr), "0" (oldval), "r" (newval), "m" (*ptr)
	    : "cc");
	return (retval);
}

/*
 * Atomically read the value pointed to by ptr.
 */

static inline unsigned long
atomic_read4(volatile atomic_t *ptr)
{
	unsigned long retval;

	asm("# atomic_read4\n"
	    "mov (%1),%0\n"
	    "# end atomic_read4"
	    : "=r" (retval)
	    : "r" (ptr), "m" (*ptr));
	return (retval);
}

/*
 * Atomically set the value pointed to by ptr to newvalue.
 */

static inline void
atomic_set4(volatile atomic_t *ptr, unsigned long newvalue)
{
	(void)atomic_xchg4(ptr, newvalue);
}

/*
 * Atomically add the addend to the value pointed to by ptr,
 * returning the old value.
 */

static inline unsigned long
atomic_xadd4(volatile atomic_t *ctr, unsigned long addend)
{
	unsigned long retval;

	asm("# atomic_xadd4\n"
	    "lock; xaddl %3,(%2)\n"
	    "# end atomic_xadd4"
	    : "=r" (retval), "=m" (*ctr)
	    : "r" (ctr), "0" (addend), "m" (*ctr)
	    : "cc");
	return (retval);
}

/*
 * Atomically exchange the new value to the value pointed to by ptr,
 * returning the old value.
 */

static inline unsigned long
atomic_xchg4(volatile atomic_t *ptr, unsigned long value)
{
	unsigned long retval;

	asm("# atomic_xchg4\n"
	    "lock; xchgl %3,(%2)\n"
	    "# end atomic_xchg4"
	    : "=r" (retval), "=m" (*ptr)
	    : "r" (ptr), "0" (value), "m" (*ptr)
	    : "cc");
	return (retval);
}

/*
 * Prevent the compiler from moving memory references across
 * a call to this function.  This does absolutely nothing
 * to prevent the CPU from reordering references -- use
 * memory_barrier() to keep both the compiler and CPU in
 * order.
 *
 * The compiler_barrier() function can be used to force
 * the compiler not refetch variable, thereby enforcing
 * a snapshot operation.
 */

static inline void
compiler_barrier(void)
{
	asm volatile("" : : : "memory");
}

/*
 * Prevent both the compiler and the CPU from moving memory
 * references across this function.
 */

static inline void
memory_barrier(void)
{
	atomic_t junk;

	(void)atomic_xchg4(&junk, 0);
	compiler_barrier();
}

/*
 * Prevent both the compiler and the CPU from moving memory
 * writes across this function.  The x86 does this by default,
 * so only need to rein in the compiler.
 */

static inline void
memory_barrier_rcu(void)
{
	compiler_barrier();
}

/*
 * Prevent both the compiler and the CPU from moving memory
 * writes across this function.  The x86 does this by default,
 * so only need to rein in the compiler.
 */

static inline void
write_barrier(void)
{
	compiler_barrier();
}

/*
 * Prevent both the compiler and the CPU from moving anything
 * across the acquisition of a spinlock that should not be so
 * moved.
 */
static inline void
spin_lock_barrier(void)
{
	compiler_barrier();
}

/*
 * Prevent both the compiler and the CPU from moving anything
 * across the acquisition of a spinlock that should not be so
 * moved.
 */
static inline void
spin_unlock_barrier(void)
{
	compiler_barrier();
}

#endif /* #ifndef _ATOMIC_H */
