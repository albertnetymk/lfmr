/*
 * Utility functions used by the program.
 */

#ifndef UTIL_H
#define UTIL_H

#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

/* Get time of day as a decimal fraction. Units are seconds.
 */

double d_gettimeofday();

/* A memory allocator which seems more reliable than glibc malloc().
 */
void *mapmem(size_t length);

#endif
