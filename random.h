#ifndef RANDOM_H
#define RANDOM_H

/* Capitalize "Random" so we don't conflict with stdlib.h. */

void init_Random();
void sRandom(unsigned long);
unsigned long Random();

#endif
