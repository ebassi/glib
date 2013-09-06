/* GLIB - Library of useful routines for C programming
 * gindexset.h: Index set
 *
 * Copyright (C) 2013  Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuele Bassi <ebassi@gnome.org>
 */

#ifndef __G_INDEX_SET_H__
#define __G_INDEX_SET_H__

#if !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

/**
 * G_RANGE_INIT:
 * @loc: the start index (counting from 0, as in C arrays)
 * @len: the number of items in the range (can be 0)
 *
 * Initializes a #GRange when declaring it.
 *
 * Since: 2.38
 */
#define G_RANGE_INIT(loc,len)   { (loc), (len) }

/**
 * G_RANGE_INIT_ZERO:
 *
 * Initializes a #GRange to 0 when declaring it.
 *
 * Since: 2.38
 */
#define G_RANGE_INIT_ZERO       G_RANGE_INIT (0, 0)

typedef struct _GRange          GRange;
typedef struct _GIndexSet       GIndexSet;

/**
 * GRange:
 * @location: the start index (counting from 0, as in C arrays)
 * @length: the number of items in the range (can be 0)
 *
 * #GRange is used to describe a portion of a series, such as characters in a
 * string, or elements in an array.
 *
 * A #GRange consists of two elements: a location and a length. The elements
 * that fall into the range lie between two points: the one represented by the
 * #GRange.location and the one represented by the #GRange.location plus the
 * #GRange.length. This means that the number of elements in the #GRange is
 * always equal to #GRange.length.
 *
 * Since: 2.38
 */
struct _GRange
{
  guint location;
  guint length;
};

GLIB_AVAILABLE_IN_2_38
GRange *        g_range_alloc                   (void);
GLIB_AVAILABLE_IN_2_38
void            g_range_free                    (GRange                  *range);
GLIB_AVAILABLE_IN_2_38
GRange *        g_range_init                    (GRange                  *range,
                                                 guint                    location,
                                                 guint                    length);
GLIB_AVAILABLE_IN_2_38
GRange *        g_range_init_with_range         (GRange                  *range,
                                                 const GRange            *source);

GLIB_AVAILABLE_IN_2_38
gboolean        g_range_equals                  (const GRange            *range_a,
                                                 const GRange            *range_b);
GLIB_AVAILABLE_IN_2_38
gboolean        g_range_contains_location       (const GRange            *range,
                                                 guint                    location);
GLIB_AVAILABLE_IN_2_38
void            g_range_union                   (const GRange            *range_a,
                                                 const GRange            *range_b,
                                                 GRange                  *result);
GLIB_AVAILABLE_IN_2_38
gboolean        g_range_intersection            (const GRange            *range_a,
                                                 const GRange            *range_b,
                                                 GRange                  *result);
GLIB_AVAILABLE_IN_2_38
guint           g_range_get_min                 (const GRange            *range);
GLIB_AVAILABLE_IN_2_38
guint           g_range_get_max                 (const GRange            *range);
GLIB_AVAILABLE_IN_2_38
guint           g_range_get_center              (const GRange            *range);

/**
 * G_INDEX_SET_NOT_FOUND:
 *
 * A sentinel value, indicating that the requested index could not be found
 * or does not exist.
 *
 * Since: 2.38
 */
#define G_INDEX_SET_NOT_FOUND \
  G_MAXINT

/**
 * GIndexSetPredicate:
 * @G_INDEX_SET_PREDICATE_LESS_THAN: Returns the closest index in a #GIndexSet
 *   that is less than a specific index
 * @G_INDEX_SET_PREDICATE_LESS_THAN_OR_EQUAL: Returns the closest index in a
 *   #GIndexSet that is less than or equal to a specific index
 * @G_INDEX_SET_PREDICATE_GREATER_THAN_OR_EQUAL: Returns the closest index in
 *   a #GIndexSet that is greater than or equal to a specific index
 * @G_INDEX_SET_PREDICATE_GREATER_THAN: Returns the closest index in a
 *   #GIndexSet that is greater than a specific index
 *
 * The predicates for g_index_set_get_index().
 *
 * Since: 2.38
 */
typedef enum {
  G_INDEX_SET_PREDICATE_LESS_THAN,
  G_INDEX_SET_PREDICATE_LESS_THAN_OR_EQUAL,
  G_INDEX_SET_PREDICATE_GREATER_THAN_OR_EQUAL,
  G_INDEX_SET_PREDICATE_GREATER_THAN
} GIndexSetPredicate;

/**
 * GIndexSetEnumerateFlags:
 * @G_INDEX_SET_ENUMERATE_NONE: No flags
 * @G_INDEX_SET_ENUMERATE_REVERSE: Specifies that the enumeration on the
 *   #GIndexSet must be performed in reverse order
 *
 * Flags for g_index_set_enumerate().
 *
 * Since: 2.38
 */
typedef enum /*< flags >*/ {
  G_INDEX_SET_ENUMERATE_NONE    = 0,
  G_INDEX_SET_ENUMERATE_REVERSE = 1 << 0,
} GIndexSetEnumerateFlags;

/**
 * GIndexSetEnumerateFunc:
 * @index_: the current index
 * @data: data passed to g_index_set_enumerate()
 *
 * A function called over each index inside a #GIndexSet.
 *
 * Returns: %TRUE if the enumeration should stop, and %FALSE otherwise
 *
 * Since: 2.38
 */
typedef gboolean (* GIndexSetEnumerateFunc) (guint    index_,
                                             gpointer data);

/* Allocation */
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_alloc               (void);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_ref                 (GIndexSet               *index_set);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_unref               (GIndexSet               *index_set);

#define g_index_set_new() \
  g_index_set_init (g_index_set_alloc ())

#define g_index_set_new_empty() \
  g_index_set_init_empth (g_index_set_alloc ());

#define g_index_set_new_for_index(idx) \
  g_index_set_init_with_index (g_index_set_alloc (), (idx))

#define g_index_set_new_for_range(range) \
  g_index_set_init_with_range (g_index_set_alloc (), (range))

/* Initialization */
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init                (GIndexSet               *index_set);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init_empty          (GIndexSet               *index_set);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init_with_index     (GIndexSet               *index_set,
                                                 guint                    index_);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init_with_indices   (GIndexSet               *index_set,
                                                 guint                    n_indices,
                                                 ...);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init_with_indicesv  (GIndexSet               *index_set,
                                                 guint                    n_indices,
                                                 const guint             *indices);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init_with_range     (GIndexSet               *index_set,
                                                 const GRange            *range);
GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_init_with_set       (GIndexSet               *index_set,
                                                 const GIndexSet         *set);

/* Query */
GLIB_AVAILABLE_IN_2_38
gboolean        g_index_set_is_mutable          (const GIndexSet         *index_set);
GLIB_AVAILABLE_IN_2_38
guint           g_index_set_get_size            (const GIndexSet         *index_set);
GLIB_AVAILABLE_IN_2_38
guint           g_index_set_get_first_index     (const GIndexSet         *index_set);
GLIB_AVAILABLE_IN_2_38
guint           g_index_set_get_last_index      (const GIndexSet         *index_set);
GLIB_AVAILABLE_IN_2_38
guint           g_index_set_get_index           (const GIndexSet         *index_set,
                                                 GIndexSetPredicate       predicate,
                                                 guint                    index_);
GLIB_AVAILABLE_IN_2_38
gboolean        g_index_set_contains_index      (const GIndexSet         *index_set,
                                                 guint                    index_);
GLIB_AVAILABLE_IN_2_38
gboolean        g_index_set_contains_range      (const GIndexSet         *index_set,
                                                 const GRange            *range);

/* Updates */
GLIB_AVAILABLE_IN_2_38
void            g_index_set_add_index           (GIndexSet               *index_set,
                                                 guint                    index_);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_add_indices         (GIndexSet               *index_set,
                                                 guint                    n_indices,
                                                 ...);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_add_indicesv        (GIndexSet               *index_set,
                                                 guint                    n_indices,
                                                 const guint             *indices);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_add_range           (GIndexSet               *index_set,
                                                 const GRange            *range);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_add_set             (GIndexSet               *index_set,
                                                 const GIndexSet         *set);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_remove_index        (GIndexSet               *index_set,
                                                 guint                    index_);

GLIB_AVAILABLE_IN_2_38
GIndexSet *     g_index_set_make_immutable      (GIndexSet               *index_set);

/* Enumeration */
GLIB_AVAILABLE_IN_2_38
void            g_index_set_enumerate           (const GIndexSet         *index_set,
                                                 GIndexSetEnumerateFlags  flags,
                                                 GIndexSetEnumerateFunc   func,
                                                 gpointer                 data);
GLIB_AVAILABLE_IN_2_38
void            g_index_set_enumerate_in_range  (const GIndexSet         *index_set,
                                                 const GRange            *range,
                                                 GIndexSetEnumerateFlags  flags,
                                                 GIndexSetEnumerateFunc   func,
                                                 gpointer                 data);

G_END_DECLS

#endif /* __G_INDEX_SET_H__ */
