/*
 * Exponential backoff.
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

#ifndef BACKOFF_H
#define BACKOFF_H

#ifdef USE_BACKOFF
void backoff_init();
void backoff_reset();
long backoff_delay();
#else
#define backoff_init()
#define backoff_reset()
#define backoff_delay()
#endif

#endif
