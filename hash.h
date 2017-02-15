/*
 * Hash table API.
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

#ifndef HASH_H
#define HASH_H

struct hash_table;

void hash_table_init(struct hash_table **h, long buckets);
void hash_table_destroy(struct hash_table **h);

int hash_search(struct hash_table *h, long key);
int hash_delete(struct hash_table *h, long key);
int hash_insert(struct hash_table *h, long key);


#endif
