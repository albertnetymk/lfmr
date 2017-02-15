/* 
 * Functions for NEW epoch-based reclamation.
 *
 * See Keir Fraser. "Practical Lock-Freedom." Ph.D. Thesis, University of 
 * Cambridge Computer Laboratory, 2003.
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

#ifndef __EPOCH_H
#define __EPOCH_H

#include "node.h"

/* Signal other threads to tell them if they need to watch you or not. */
void nebr_set_active();
void nebr_set_inactive();

/* Begin and end lockless ops. */
void lockless_begin();
void lockless_end();

/* 
 * Call this *directly* if and only if we're out of memory. 
 */
void update_epoch();

/* 
 * Since multiple algorithms could use epoch-based reclamation, factor the
 * initialization code into a function they can all call.
 */
void init_epoch();

/*
 * Make epoch-based callbacks have the same interface as with RCU.
 */
void free_node_later(node_t *q);

#endif
