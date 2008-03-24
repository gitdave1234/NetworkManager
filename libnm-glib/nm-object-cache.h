/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
 *
 * Dan Williams <dcbw@redhat.com>
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
 * (C) Copyright 2008 Red Hat, Inc.
 */

#ifndef NM_OBJECT_CACHE_H
#define NM_OBJECT_CACHE_H

#include <glib/gtypes.h>
#include <glib-object.h>
#include "nm-object.h"

G_BEGIN_DECLS

NMObject *nm_object_cache_get (const char *path);
void nm_object_cache_add (NMObject *object);
void nm_object_cache_remove_by_object (NMObject *object);
void nm_object_cache_remove_by_path (const char *path);

G_END_DECLS

#endif /* NM_OBJECT_CACHE_H */
