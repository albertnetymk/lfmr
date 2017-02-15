/*
 * Lock-free reference counting API taken from:
 *
 * Michael, M. M. and Scott, M. L. 1995 Correction of a Memory Management Method 
 * for Lock-Free Data Structures. Technical Report. UMI Order Number: TR599., 
 * University of Rochester.
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
 * Copyright (c) 2005 Tom Hart.
 */

#include "node.h"

#ifndef LFRC_H
#define LFRC_H

void lfrc_refcnt_inc(node_t *p);
void lfrc_refcnt_dec(node_t *p);
node_t *safe_read(node_t **p);

#endif
