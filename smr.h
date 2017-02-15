/*
 * Functionality for hazard pointers, aka safe memory reclamation (SMR).
 *
 * Uses a linked list for the hazard poitners. Using an array can be a 
 * related experiment.
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
 
#ifndef __SMR_H
#define __SMR_H

#include "mr.h"
#include "node.h"
#include "test.h"

/* Parameters to the algorithm:
 *  K: Number of hazard pointers per CPU.
 *  H: Number of hazard pointers required.
 *  R: Chosen such that R = H + Omega(H).
 */
#define K 2
#define H (K * tg->nthreads)
#define R (100 + 2*H)

typedef struct hazard_pointer_s {
	struct node *  __attribute__ ((__aligned__ (CACHESIZE))) p;
} hazard_pointer;
 
/* Must be dynamically initialized to be an array of size H. */
hazard_pointer *HP;
 
void scan();

void free_node_later(node_t *);
 
#endif
