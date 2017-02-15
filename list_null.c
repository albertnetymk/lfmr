/*
 * Null linked list to evaluate the overhead of Random() and the test loop.
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
 * Copyright (c) 2005 Tom Hart.
 */

#include <stdio.h>
#include <string.h>
#include "spinlock.h"
#include "smr.h"
#include "list.h"
#include "node.h"
#include "allocator.h"
#include "util.h"
#include <stdio.h>

struct list {
};

void list_init(struct list **l)
{
}

void list_destroy(struct list **l)
{
}

int search(struct list *l, long key)
{	
}

int delete(struct list *l, long key)
{
}

int insert(struct list *l, long key)
{
}
