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

double avg(double *data, int samples);
double stdev(double *data, int samples);
double max(double *data, int samples);
double min(double *data, int samples);

/* A memory allocator which seems more reliable than glibc malloc().
 */
void *mapmem(size_t length);

#endif
