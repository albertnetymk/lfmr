/*
 * Atomic operations for 32-bit PPC.  Will run in 32-bit mode on
 * 64-bit PPC.  Using 32-bit to permit more effective scrounging
 * of machine time.  ;-)
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

#ifndef _ATOMIC_H
#define _ATOMIC_H
#define _ATOMIC_H_PPC32

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
	    "lwarx	%0,0,%2\n"
	    "cmpw	cr0,%0,%3\n"
	    "bne-	$+12\n"
	    "stwcx.	%4,0,%2\n"
	    "bne-	$-16\n"
	    "# end atomic_cmpxchg4"
	    : "=&r" (retval), "+m" (*ptr)
	    : "r" (ptr), "r" (oldval), "r" (newval), "m" (*ptr)
	    : "cr0");
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
	    "lwzx	%0,0,%1\n"
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
	unsigned long tmp;

	asm("# atomic_xadd4\n"
	    "lwarx	%1,0,%3\n"
	    "add	%0,%4,%1\n"
	    "stwcx.	%0,0,%3\n"
	    "bne-	$-12\n"
	    "# end atomic_xadd4"
	    : "=&r" (tmp), "=&r" (retval), "=m" (*ctr)
	    : "r" (ctr), "r" (addend), "m" (*ctr));
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
	    "lwarx	%0,0,%2\n"
	    "stwcx.	%3,0,%2\n"
	    "bne-	$-8\n"
	    "# end atomic_xchg4"
	    : "=&r" (retval), "=m" (*ptr)
	    : "r" (ptr), "r" (value), "m" (*ptr)
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
	asm volatile("eieio" : : : "memory");
}

/*
 * Prevent both the compiler and the CPU from moving memory
 * writes across this function.
 */

static inline void
memory_barrier_rcu(void)
{
	asm volatile("eieio" : : : "memory");
}

/*
 * Prevent both the compiler and the CPU from moving memory
 * writes across this function.
 */

static inline void
write_barrier(void)
{
	asm volatile("eieio" : : : "memory");
}

/*
 * Prevent both the compiler and the CPU from moving anything
 * across the acquisition of a spinlock that should not be so
 * moved.
 */
static inline void
spin_lock_barrier(void)
{
	asm volatile("isync" : : : "memory");
}

/*
 * Prevent both the compiler and the CPU from moving anything
 * across the acquisition of a spinlock that should not be so
 * moved.
 */
static inline void
spin_unlock_barrier(void)
{
	/* asm volatile("lwsync" : : : "memory"); PowerPC manual */
	asm volatile("eieio" : : : "memory");
}

#endif /* #ifndef _ATOMIC_H */
