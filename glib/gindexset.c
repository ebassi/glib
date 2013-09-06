/* GLIB - Library of useful routines for C programming
 * gindexset.c: Index set
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

#include "config.h"

#include <string.h>

#include "gindexset.h"

#include "gatomic.h"
#include "gslice.h"
#include "gmem.h"
#include "gtestutils.h"
#include "gmessages.h"

#define VERBOSE 1

/**
 * SECTION:gindexset
 * @title: GIndexSet
 * @short_description: Index sets
 *
 * #GIndexSet is a collection of unique unsigned integers, also known as
 * indices.
 *
 * Indices are used to access collections like #GArray, and you can use
 * a #GIndexSet to identify a subset of indices in that array.
 *
 * You should not use #GIndexSet to store an arbitrary list of unsigned
 * integers; index sets are stored as sorted ranges for efficiency, which
 * also means that an index can only appear once inside an index set.
 *
 * Index sets are allocated using g_index_set_alloc(), and initialized
 * using one of the initialization functions; a #GIndexSet instance can
 * be initialized multiple times, allowing reuse of the same resources
 * for different index sets. #GIndexSet is also reference counted: use
 * g_index_set_ref() to acquire a reference, and g_index_set_unref() to
 * release it. If g_index_set_unref() releases the last reference on an
 * index set instance, the resources allocatd by g_index_set_alloc() will
 * be released.
 *
 * If a #GIndexSet is initialized as empty, or with a list of indices, it
 * is immediately set as immutable: no further modifications are allowed
 * on the index set after the initialization function returns. If you want
 * to add indices to a #GIndexSet using g_index_set_add_index(), or remove
 * indices using g_index_set_remove_index(), you must call g_index_set_init()
 * to initialize a mutable #GIndexSet instance; once you finished adding
 * the desired indices, call g_index_set_make_immutable(). You can check
 * if a #GIndexSet instance is mutable by using g_index_set_is_mutable().
 * It is possible to reinitialize any #GIndexSet instance to make it mutable,
 * though its current contents will be lost.
 *
 * To enumerate the contents of a #GIndexSet you can either use the
 * g_index_set_get_index() function in a while loop, for instance:
 *
 * |[
 *   /&ast; iterate forwards &ast;/
 *   guint cur_index = g_index_set_get_first_index (index_set);
 *   while (cur_index != G_INDEX_SET_NOT_FOUND)
 *     {
 *       ...
 *       cur_index =
 *         g_index_set_get_index (index_set,
 *                                G_INDEX_SET_PREDICATE_GREATER_THAN,
 *                                cur_index);
 *     }
 *
 *   /&ast; iterate backwards &ast;/
 *   guint cur_index = g_index_set_get_last_index (index_set);
 *   while (cur_index != G_INDEX_SET_NOT_FOUND)
 *     {
 *       ...
 *       cur_index =
 *         g_index_set_get_index (index_set,
 *                                G_INDEX_SET_PREDICATE_LESS_THAN,
 *                                cur_index);
 *     }
 * ]|
 *
 * or you can use the g_index_set_enumerate_in_range() and the
 * g_index_set_enumerate() functions.
 */

/*** GRange ***/

/**
 * g_range_alloc: (constructor)
 *
 * Allocates a new #GRange.
 *
 * Returns: (transfer full): the newly allocated #GRange. Use
 *   g_range_free() to release the resources allocated by this
 *   function
 *
 * Since: 2.38
 */
GRange *
g_range_alloc (void)
{
  return g_slice_new0 (GRange);
}

/**
 * g_range_free:
 * @range: a #GRange
 *
 * Frees the resources allocated by g_range_alloc().
 *
 * Since: 2.38
 */
void
g_range_free (GRange *range)
{
  if (range != NULL)
    g_slice_free (GRange, range);
}

/**
 * g_range_init:
 * @range: a #GRange
 * @location: the start index (counting from 0, like in C arrays)
 * @length: the number of items in the range (can be 0)
 *
 * Initializes @range using @location and @length.
 *
 * Returns: (transfer none): the initialized #GRange
 *
 * Since: 2.38
 */
GRange *
g_range_init (GRange *range,
              guint   location,
              guint   length)
{
  g_return_val_if_fail (range != NULL, NULL);

  range->location = location;
  range->length = length;

  return range;
}

/**
 * g_range_init_with_range:
 * @range: a #GRange
 * @source: a #GRange
 *
 * Initializes @range using the contents of @source.
 *
 * Return value: (transfer none): the initialized #GRange
 *
 * Since: 2.38
 */
GRange *
g_range_init_with_range (GRange       *range,
                         const GRange *source)
{
  g_return_val_if_fail (range != NULL, NULL);

  *range = *source;

  return range;
}

/**
 * g_range_equals:
 * @range_a: a #GRange
 * @range_b: a #GRange
 *
 * Checks if @range_a is equal to @range_b.
 *
 * Returns: %TRUE if the ranges are equal, and %FALSE otherwise
 *
 * Since: 2.38
 */
gboolean
g_range_equals (const GRange *range_a,
                const GRange *range_b)
{
  if (range_a == range_b)
    return TRUE;

  if (range_a == NULL || range_b == NULL)
    return FALSE;

  return range_b->location == range_a->location &&
         range_b->length == range_a->length;
}

/**
 * g_range_contains_location:
 * @range: a #GRange
 * @location: a position to check
 *
 * Checks whether @location is contained inside @range.
 *
 * Returns: %TRUE if the location is inside the #GRange, and %FALSE otherwise
 *
 * Since: 2.38
 */
gboolean
g_range_contains_location (const GRange *range,
                           guint         location)
{
  g_return_val_if_fail (range != NULL, FALSE);

  if (location >= range->location && location < (range->location + range->length))
    return TRUE;

  return FALSE;
}

/**
 * g_range_union:
 * @range_a: a #GRange
 * @range_b: a #GRange
 * @result: (out caller-allocates): return location for the
 *   result of the union
 *
 * Creates a #GRange expressing the union of @range_a and @range_b.
 *
 * Since: 2.38
 */
void
g_range_union (const GRange *range_a,
               const GRange *range_b,
               GRange       *result)
{
  GRange res;

  g_return_if_fail (range_a != NULL);
  g_return_if_fail (range_b != NULL);

  res.location = MIN (range_a->location, range_b->location);
  res.length = MAX (range_a->location + range_a->length,
                    range_b->location + range_b->length)
             - res.location;

  if (result != NULL)
    *result = res;
}

/**
 * g_range_intersection:
 * @range_a: a #GRange
 * @range_b: a #GRange
 * @result: (allow-none) (out caller-allocates): return location for
 *   the result of the intersection, or %NULL
 *
 * Creates a #GRange expressing the intersection of @range_a and @range_b.
 *
 * This function can be used to quickly check if two #GRange overlap, by
 * passing %NULL for @result, and checking the return value.
 *
 * Returns: %TRUE if the intersection of two #GRange is not empty, and
 *   %FALSE otherwise
 *
 * Since: 2.38
 */
gboolean
g_range_intersection (const GRange *range_a,
                      const GRange *range_b,
                      GRange       *result)
{
  GRange res;

  g_return_val_if_fail (range_a != NULL, FALSE);
  g_return_val_if_fail (range_b != NULL, FALSE);

  res.location = MAX (range_a->location, range_b->location);
  res.length = MIN (range_a->location + range_a->length,
                    range_b->location + range_b->length);

  if (res.length > res.location)
    res.length -= res.location;
  else
    return FALSE;

  if (result != NULL)
    *result = res;

  return TRUE;
}

/**
 * g_range_get_min:
 * @range: a #GRange
 *
 * Retrieves the minimum value of the @range.
 *
 * Returns: the minimum value of the #GRange
 *
 * Since: 2.38
 */
guint
g_range_get_min (const GRange *range)
{
  g_return_val_if_fail (range != NULL, 0);

  return range->location;
}

/**
 * g_range_get_max:
 * @range: a #GRange
 *
 * Retrieves the maximum value of the @range.
 *
 * Returns: the maximum value of the #GRange
 *
 * Since: 2.38
 */
guint
g_range_get_max (const GRange *range)
{
  g_return_val_if_fail (range != NULL, 0);

  return range->location + range->length;
}

/**
 * g_range_get_center:
 * @range: a #GRange
 *
 * Retrieves the value at the center of the @range.
 *
 * Returns: the center value of the #GRange
 *
 * Since: 2.38
 */
guint
g_range_get_center (const GRange *range)
{
  g_return_val_if_fail (range != NULL, 0);

  return range->location + (range->length / 2);
}

/*** GIndexSet ***/

struct _GIndexSet
{
  volatile int ref_count;

  GArray *ranges;

  guint is_mutable : 1;
};

static GRange *
g_index_set_get_range (const GIndexSet *index_set,
                       guint            position)
{
  if (index_set->ranges == NULL || position >= index_set->ranges->len)
    return NULL;

  return &g_array_index (index_set->ranges, GRange, position);
}

static inline guint
g_index_set_get_position (const GIndexSet *index_set,
                          guint            index_)
{
  const GRange *range;
  guint upper = index_set->ranges->len;
  guint lower = 0;
  guint pos;

  /* binary search through the ranges */
  for (pos = upper / 2; upper != lower; pos = (upper + lower) / 2)
    {
      range = g_index_set_get_range (index_set, pos);

      if (index_ < range->location)
        upper = pos;
      else if (index_ > (range->location + range->length))
        lower = pos + 1;
      else
        break;
    }

  /* skip ranges that contain values smaller than the index */
  while (pos < index_set->ranges->len &&
         index_ >= g_range_get_max (g_index_set_get_range (index_set, pos)))
    {
      pos += 1;
    }

  return pos;
}

static inline void
g_index_set_clear_ranges (GIndexSet *index_set)
{
  if (index_set->ranges == NULL)
    index_set->ranges = g_array_new (FALSE, FALSE, sizeof (GRange));
  else
    g_array_set_size (index_set->ranges, 0);

  index_set->is_mutable = TRUE;
}

#ifdef VERBOSE
static void
g_index_set_print (const GIndexSet *index_set)
{
  GString *buf = g_string_new (NULL);

  if (index_set->ranges == NULL || index_set->ranges->len == 0)
    g_string_append (buf, "[ empty ]");
  else
    {
      guint i;

      g_string_append_printf (buf, "[ number of indices %u (in %u ranges), indices:",
                              g_index_set_get_size (index_set),
                              index_set->ranges->len);

      for (i = 0; i < index_set->ranges->len; i++)
        {
          const GRange *r = g_index_set_get_range (index_set, i);

          if (r->length == 1)
            g_string_append_printf (buf, " %u", r->location);
          else
            g_string_append_printf (buf, " (%u - %u)", r->location, r->length);
        }

      g_string_append (buf, " ]");
    }

  g_print ("*** %s\n", buf->str);
  g_string_free (buf, TRUE);
}
#endif /* VERBOSE */

#ifdef G_DISABLE_ASSERT
#define g_index_set_ensure_ranges(i)
#else
static inline void
g_index_set_ensure_ranges (const GIndexSet *index_set)
{
  guint i, last = 0;

#ifdef VERBOSE
  g_index_set_print (index_set);
#endif

  if (index_set->ranges == NULL)
    return;

  for (i = 0; i < index_set->ranges->len; i++)
    {
      const GRange *range = g_index_set_get_range (index_set, i);

      /* ensure that the ranges are sorted in ascending order */
      if (i > 0)
        g_assert (range->location > last);
      else
        g_assert (range->location >= last);

      /* ensure that there are no empty ranges */
      g_assert (g_range_get_max (range) > range->location);

      last = g_range_get_max (range);
    }
}
#endif /* G_DISABLE_ASSERT */

/**
 * g_index_set_alloc:
 *
 * Allocates a new #GIndexSet.
 *
 * The returned index set must be initialized before being used.
 *
 * Returns: (transfer full): the newly allocated #GIndexSet.
 *   Use g_index_set_unref() when done, to release the resources
 *   allocated by this function
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_alloc (void)
{
  GIndexSet *set = g_slice_new (GIndexSet);

  set->ref_count = 1;
  set->ranges = NULL;
  set->is_mutable = TRUE;

  return set;
}

/**
 * g_index_set_new:
 *
 * A convenience macro that expands to g_index_set_alloc() and
 * g_index_set_init().
 *
 * Since: 2.38
 */

/**
 * g_index_set_new_empty:
 *
 * A convenience macro that expands to g_index_set_alloc() and
 * g_index_set_init_empty().
 *
 * Since: 2.38
 */

/**
 * g_index_set_new_for_index:
 * @idx: an index
 *
 * A convenience macro that expands to g_index_set_alloc() and
 * g_index_set_init_with_index().
 *
 * Since: 2.38
 */

/**
 * g_index_set_new_for_range:
 * @range: a #GRange
 *
 * A convenience macro that expands to g_index_set_alloc() and
 * g_index_set_init_with_range().
 *
 * Since: 2.38
 */

/**
 * g_index_set_ref:
 * @index_set: a #GIndexSet
 *
 * Acquires a reference on @index_set.
 *
 * Returns: (transfer none): the #GIndexSet, with its reference count
 *   increased by one
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_ref (GIndexSet *index_set)
{
  g_return_val_if_fail (index_set != NULL, NULL);

  g_atomic_int_add (&index_set->ref_count, 1);

  return index_set;
}

/**
 * g_index_set_unref:
 * @index_set:
 *
 * Releases a reference on @index_set.
 *
 * If the reference released was the last one, the resources allocated by
 * the #GIndexSet will be released.
 *
 * Since: 2.38
 */
void
g_index_set_unref (GIndexSet *index_set)
{
  g_return_if_fail (index_set != NULL);

  if (g_atomic_int_dec_and_test (&index_set->ref_count))
    {
      if (index_set->ranges != NULL)
        g_array_unref (index_set->ranges);

      g_slice_free (GIndexSet, index_set);
    }
}

/**
 * g_index_set_init:
 * @index_set: a #GIndexSet
 *
 * Initializes a #GIndexSet.
 *
 * This function is used to initialize a mutable #GIndexSet in order to use
 * g_index_set_add_index(). Once the desired indices have been added to the
 * index set, use g_index_set_make_immutable() to prevent further writes.
 *
 * Returns: the initialized, mutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init (GIndexSet *index_set)
{
  g_return_val_if_fail (index_set != NULL, NULL);

  g_index_set_clear_ranges (index_set);

  return index_set;
}

/**
 * g_index_set_init_empty:
 * @index_set: a #GIndexSet
 *
 * Initializes @index_set to be empty, and makes it immutable.
 *
 * Returns: the initialized, immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init_empty (GIndexSet *index_set)
{
  g_return_val_if_fail (index_set != NULL, NULL);

  g_index_set_clear_ranges (index_set);

  return g_index_set_make_immutable (index_set);
}

static inline void
g_index_set_add_range_internal (GIndexSet    *index_set,
                                const GRange *range)
{
  guint pos;

  if (range->length == 0)
    return;

  pos = g_index_set_get_position (index_set, range->location);
  if (pos >= index_set->ranges->len)
    {
      GRange copy;

      g_range_init_with_range (&copy, range);
      g_array_append_val (index_set->ranges, copy);
    }
  else
    {
      GRange *r = g_index_set_get_range (index_set, pos);
      GRange copy;

      if (g_range_contains_location (r, range->location))
        pos += 1;

      g_range_init_with_range (&copy, range);
      g_array_insert_val (index_set->ranges, pos, copy);
    }

  /* coalesce the preceding range intervals */
  while (pos > 0)
    {
      GRange *r = g_index_set_get_range (index_set, pos - 1);

      if (g_range_get_max (r) < range->location)
        break;

      if (g_range_get_max (r) >= g_range_get_max (range))
        g_array_remove_index (index_set->ranges, pos--);
      else
        {
          r->length += (g_range_get_max (range) - g_range_get_max (r));
          g_array_remove_index (index_set->ranges, pos--);
        }
    }

  /* and now coalesce the following intervals */
  while (pos + 1 < index_set->ranges->len)
    {
      GRange *r = g_index_set_get_range (index_set, pos + 1);
      GRange copy;

      if (g_range_get_max (range) < r->location)
        break;

      g_range_init_with_range (&copy, r);
      g_array_remove_index (index_set->ranges, pos + 1);
      if (g_range_get_max (&copy) > g_range_get_max (range))
        {
          int offset = g_range_get_max (&copy) - g_range_get_max (range);

          r = g_index_set_get_range (index_set, pos);
          r->length += offset;
        }
    }

  g_index_set_ensure_ranges (index_set);
}

/**
 * g_index_set_init_with_index:
 * @index_set: a #GIndexSet
 * @index_: an index
 *
 * Initializes @index_set with @index_, and makes it immutable.
 *
 * Returns: the initialized, immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init_with_index (GIndexSet *index_set,
                             guint      index_)
{
  GRange range = G_RANGE_INIT (index_, 1);

  g_return_val_if_fail (index_set != NULL, NULL);

  g_index_set_clear_ranges (index_set);

  g_array_insert_val (index_set->ranges, 0, range);

  return g_index_set_make_immutable (index_set);
}

/**
 * g_index_set_init_with_indicesv:
 * @index_set: a #GIndexSet
 * @n_indices: the number of indices inside @indices
 * @indices: (array length=n_indices): an array of indices
 *
 * Initializes @index_set with an array of indices, and makes it immutable.
 *
 * Rename to: g_index_set_init_with_indices
 *
 * Returns: the initialized, immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init_with_indicesv (GIndexSet   *index_set,
                                guint        n_indices,
                                const guint *indices)
{
  guint i;

  g_return_val_if_fail (index_set != NULL, NULL);

  g_index_set_clear_ranges (index_set);

  if (n_indices > 0)
    g_return_val_if_fail (indices != NULL, index_set);

  for (i = 0; i < n_indices; i++)
    {
      GRange range = G_RANGE_INIT (indices[i], 1);

      g_index_set_add_range_internal (index_set, &range);
    }

  return g_index_set_make_immutable (index_set);
}

/**
 * g_index_set_init_with_indices: (skip)
 * @index_set: a #GIndexSet
 * @n_indices: the number of indices to add
 * @...: a list of indices, of length @n_indices, to be added to @index_set
 *
 * Initializes @index_set with a list of indices, and makes it immutable.
 *
 * Returns: the initialized, immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init_with_indices (GIndexSet *index_set,
                               guint      n_indices,
                               ...)
{
  va_list args;
  guint i = 0;

  g_return_val_if_fail (index_set != NULL, NULL);

  g_index_set_clear_ranges (index_set);

  va_start (args, n_indices);

  while (i < n_indices)
    {
      guint idx = va_arg (args, guint);
      GRange range = G_RANGE_INIT (idx, 1);

      g_index_set_add_range_internal (index_set, &range);

      i += 1;
    }

  va_end (args);

  return g_index_set_make_immutable (index_set);
}

/**
 * g_index_set_init_with_range:
 * @index_set: a #GIndexSet
 * @range: a #GRange
 *
 * Initializes @index_set with all the indices inside @range, and makes it
 * immutable.
 *
 * Returns: the initialized, immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init_with_range (GIndexSet    *index_set,
                             const GRange *range)
{
  GRange copy;

  g_return_val_if_fail (index_set != NULL, NULL);
  g_return_val_if_fail (range != NULL, index_set);

  g_index_set_clear_ranges (index_set);

  g_range_init_with_range (&copy, range);
  g_index_set_add_range_internal (index_set, &copy);

  return g_index_set_make_immutable (index_set);
}

/**
 * g_index_set_init_with_set:
 * @index_set: a #GIndexSet
 * @set: a #GIndexSet
 *
 * Initializes @index_set with all the indices inside @set, and makes it
 * immutable.
 *
 * Returns: the initialized, immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_init_with_set (GIndexSet       *index_set,
                           const GIndexSet *set)
{
  guint i;

  g_return_val_if_fail (index_set != NULL, NULL);
  g_return_val_if_fail (set != NULL, index_set);

  g_index_set_clear_ranges (index_set);

  if (set->ranges == NULL || set->ranges->len == 0)
    return g_index_set_make_immutable (index_set);

  for (i = 0; i < set->ranges->len; i++)
    {
      const GRange *r = g_index_set_get_range (index_set, i);

      g_index_set_add_range_internal (index_set, r);
    }

  return g_index_set_make_immutable (index_set);
}

/**
 * g_index_set_make_immutable:
 * @index_set: a #GIndexSet
 *
 * Makes @index_set immutable.
 *
 * Returns: the immutable #GIndexSet
 *
 * Since: 2.38
 */
GIndexSet *
g_index_set_make_immutable (GIndexSet *index_set)
{
  g_return_val_if_fail (index_set != NULL, NULL);

  index_set->is_mutable = FALSE;

  return index_set;
}

/**
 * g_index_set_is_mutable:
 * @index_set: a #GIndexSet
 *
 * Checks if @index_set is mutable.
 *
 * Returns: %TRUE if the #GIndexSet is mutable, and %FALSE otherwise
 *
 * Since: 2.38
 */
gboolean
g_index_set_is_mutable (const GIndexSet *index_set)
{
  g_return_val_if_fail (index_set != NULL, FALSE);

  return index_set->is_mutable;
}

/**
 * g_index_set_get_size:
 * @index_set: a #GIndexSet
 *
 * Retrieves the number of indices inside @index_set.
 *
 * Returns: the number of indices insie the #GIndexSet
 *
 * Since: 2.38
 */
guint
g_index_set_get_size (const GIndexSet *index_set)
{
  guint count = 0;
  guint i;

  g_return_val_if_fail (index_set != NULL, 0);

  if (index_set->ranges == NULL ||
      index_set->ranges->len == 0)
    return 0;

  for (i = 0; i < index_set->ranges->len; i++)
    {
      const GRange *range = g_index_set_get_range (index_set, i);

      count += range->length;
    }

  return count;
}

/**
 * g_index_set_get_first_index:
 * @index_set: a #GIndexSet
 *
 * Retrieves the first index inside @index_set.
 *
 * If the #GIndexSet has only one index, the first index is also
 * the last index.
 *
 * Returns: the first index inside the #GIndexSet
 *
 * Since: 2.38
 */
guint
g_index_set_get_first_index (const GIndexSet *index_set)
{
  g_return_val_if_fail (index_set != NULL, G_INDEX_SET_NOT_FOUND);

  if (index_set->ranges == NULL ||
      index_set->ranges->len == 0)
    return G_INDEX_SET_NOT_FOUND;

  return g_index_set_get_range (index_set, 0)->location;
}

/**
 * g_index_set_get_last_index:
 * @index_set: a #GIndexSet
 *
 * Retrieves the last index inside @index_set.
 *
 * If the #GIndexSet has only one index, the last index is also
 * the first index.
 *
 * Returns: the last index inside the #GIndexSet
 *
 * Since: 2.38
 */
guint
g_index_set_get_last_index (const GIndexSet *index_set)
{
  const GRange *range;

  g_return_val_if_fail (index_set != NULL, G_INDEX_SET_NOT_FOUND);

  if (index_set->ranges == NULL ||
      index_set->ranges->len == 0)
    return G_INDEX_SET_NOT_FOUND;

  range = g_index_set_get_range (index_set, index_set->ranges->len - 1);

  return g_range_get_max (range) - 1;
}

/**
 * g_index_set_get_index:
 * @index_set: a #GIndexSet
 * @predicate: the predicate to use to retrieve the index
 * @index_: the index being queried
 *
 * Retrieves the first index satisfying the @predicate inside @index_set.
 *
 * Returns: the first index satisfying the predicate, or %G_INDEX_SET_NOT_FOUND
 *
 * Since: 2.38
 */
guint
g_index_set_get_index (const GIndexSet    *index_set,
                       GIndexSetPredicate  predicate,
                       guint               index_)
{
  const GRange *r;
  guint pos;

  g_return_val_if_fail (index_set != NULL, G_INDEX_SET_NOT_FOUND);

  if (index_set->ranges == NULL || index_set->ranges->len == 0)
    return G_INDEX_SET_NOT_FOUND;

  switch (predicate)
    {
    case G_INDEX_SET_PREDICATE_GREATER_THAN:
      if (index_ + 1 == G_INDEX_SET_NOT_FOUND)
        return G_INDEX_SET_NOT_FOUND;
      else
        index_ += 1;

      /* fall through */
    case G_INDEX_SET_PREDICATE_GREATER_THAN_OR_EQUAL:
      if (index_ == G_INDEX_SET_NOT_FOUND)
        return G_INDEX_SET_NOT_FOUND;

      pos = g_index_set_get_position (index_set, index_);
      if (pos >= index_set->ranges->len)
        return G_INDEX_SET_NOT_FOUND;

      r = g_index_set_get_range (index_set, pos);
      if (g_range_contains_location (r, index_))
        return index_;

      return r->location;

    case G_INDEX_SET_PREDICATE_LESS_THAN:
      if (index_ == 0)
        return G_INDEX_SET_NOT_FOUND;
      else
        index_ -= 1;

      /* fall through */
    case G_INDEX_SET_PREDICATE_LESS_THAN_OR_EQUAL:
      pos = g_index_set_get_position (index_set, index_);
      if (pos >= index_set->ranges->len)
        return G_INDEX_SET_NOT_FOUND;

      r = g_index_set_get_range (index_set, pos);
      if (g_range_contains_location (r, index_))
        return index_;

      if (pos == 0)
        return G_INDEX_SET_NOT_FOUND;
      else
        pos -= 1;

      r = g_index_set_get_range (index_set, pos);
      return g_range_get_max (r) - 1;

    default:
      break;
    }

  return G_INDEX_SET_NOT_FOUND;
}

/**
 * g_index_set_contains_index:
 * @index_set: a #GIndexSet
 * @index_: the index to check
 *
 * Checks if @index_ is contained inside @index_set.
 *
 * Returns: %TRUE if the desired index is contained inside a #GIndexSet,
 *   and %FALSE otherwise
 *
 * Since: 2.38
 */
gboolean
g_index_set_contains_index (const GIndexSet *index_set,
                            guint            index_)
{
  const GRange *range;
  guint pos;

  g_return_val_if_fail (index_set != NULL, FALSE);

  if (index_set->ranges == NULL ||
      index_set->ranges->len == 0)
    return FALSE;

  pos = g_index_set_get_position (index_set, index_);
  if (pos >= index_set->ranges->len)
    return FALSE;

  range = g_index_set_get_range (index_set, pos);

  return g_range_contains_location (range, index_);
}

/**
 * g_index_set_contains_range:
 * @index_set: a #GIndexSet
 * @range: a #GRange
 *
 * Checks if @range is fully contained inside @index_set.
 *
 * Returns: %TRUE if the #GRange of indices is fully contained inside
 *   the #GIndexSet, and %FALSE otherwise
 *
 * Since: 2.38
 */
gboolean
g_index_set_contains_range (const GIndexSet *index_set,
                            const GRange    *range)
{
  const GRange *r;
  guint pos;

  g_return_val_if_fail (index_set != NULL, FALSE);
  g_return_val_if_fail (range != NULL, FALSE);

  if (range->length == 0)
    return TRUE;

  /* if the range crosses G_INDEX_SET_NOT_FOUND then we abort */
  if (range->location > G_INDEX_SET_NOT_FOUND - range->length)
    return FALSE;

  if (index_set->ranges == NULL || index_set->ranges->len == 0)
    return FALSE;

  pos = g_index_set_get_position (index_set, range->location);
  if (pos >= index_set->ranges->len)
    return FALSE;

  r = g_index_set_get_range (index_set, pos);
  if (g_range_contains_location (r, range->location) &&
      g_range_contains_location (r, g_range_get_max (range) - 1))
    return TRUE;

  return FALSE;
}

/**
 * g_index_set_add_index:
 * @index_set: a #GIndexSet
 * @index_: the index to add
 *
 * Adds @index_ to a mutable @index_set.
 *
 * Since: 2.38
 */
void
g_index_set_add_index (GIndexSet *index_set,
                       guint      index_)
{
  GRange range;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (g_index_set_is_mutable (index_set));

  g_range_init (&range, index_, 1);
  g_index_set_add_range_internal (index_set, &range);
}

/**
 * g_index_set_add_indices: (skip)
 * @index_set: a #GIndexSet
 * @n_indices: the number of indices to add
 * @...: a list of indices to add
 *
 * Adds @n_indices to a mutable @index_set.
 *
 * Since: 2.38
 */
void
g_index_set_add_indices (GIndexSet *index_set,
                         guint      n_indices,
                         ...)
{
  va_list args;
  guint i = 0;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (g_index_set_is_mutable (index_set));

  va_start (args, n_indices);
  while (i < n_indices)
    {
      guint idx = va_arg (args, guint);
      GRange range = G_RANGE_INIT (idx, 1);

      g_index_set_add_range_internal (index_set, &range);

      i += 1;
    }
  va_end (args);
}

/**
 * g_index_set_add_indicesv:
 * @index_set: a #GIndexSet
 * @n_indices: the number of indices to add
 * @indices: (array length=n_indices): a C array containing the
 *   indices to add
 *
 * Adds @n_indices to a mutable @index_set.
 *
 * Rename to: g_index_set_add_indices
 *
 * Since: 2.38
 */
void
g_index_set_add_indicesv (GIndexSet   *index_set,
                          guint        n_indices,
                          const guint *indices)
{
  guint i;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (g_index_set_is_mutable (index_set));

  if (n_indices > 0)
    g_return_if_fail (indices != NULL);

  for (i = 0; i < n_indices; i++)
    {
      GRange range = G_RANGE_INIT (indices[i], 1);

      g_index_set_add_range_internal (index_set, &range);
    }
}

/**
 * g_index_set_add_range:
 * @index_set: a #GIndexSet
 * @range: a #GRange
 *
 * Adds the indices in @range to a mutable @index_set.
 *
 * Since: 2.38
 */
void
g_index_set_add_range (GIndexSet    *index_set,
                       const GRange *range)
{
  GRange copy;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (g_index_set_is_mutable (index_set));
  g_return_if_fail (range != NULL);

  g_range_init_with_range (&copy, range);
  g_index_set_add_range_internal (index_set, &copy);
}

/**
 * g_index_set_add_set:
 * @index_set: a #GIndexSet
 * @set: a #GIndexSet
 *
 * Adds the indices in @set to a mutable @index_set.
 *
 * Since: 2.38
 */
void
g_index_set_add_set (GIndexSet       *index_set,
                     const GIndexSet *set)
{
  guint i;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (g_index_set_is_mutable (index_set));
  g_return_if_fail (set != NULL);

  if (set->ranges == NULL || set->ranges->len == 0)
    return;

  for (i = 0; i < set->ranges->len; i++)
    {
      const GRange *r = g_index_set_get_range (set, i);

      g_index_set_add_range_internal (index_set, r);
    }
}

static void
g_index_set_remove_range (GIndexSet    *index_set,
                          const GRange *range)
{
  guint pos;

  if (index_set->ranges == NULL || index_set->ranges->len == 0)
    return;

  pos = g_index_set_get_position (index_set, index_);
  if (pos >= index_set->ranges->len)
    return;
}

/**
 * g_index_set_remove_index:
 * @index_set: a #GIndexSet
 * @index_: the index to remove
 *
 * Removes @index_ from the @index_set.
 *
 * Since: 2.38
 */
void
g_index_set_remove_index (GIndexSet *index_set,
                          guint      index_)
{
  GRange range = G_RANGE_INIT (index_, 1);

  g_return_if_fail (index_set != NULL);

  g_index_set_remove_range (index_set, &range);
}

/**
 * g_index_set_enumerate_in_range:
 * @index_set: a #GIndexSet
 * @range: a #GRange
 * @flags: flags controlling the enumeration
 * @func: (scope call): a function to be called on every element of the set
 * @data: (closure): data to be passed to @func
 *
 * Calls @func on all elements inside the @index_set within the given @range.
 * The function should return %FALSE to continue the enumeration, or %TRUE to
 * stop it.
 *
 * The @flags can be used to control the direction of the enumeration.
 *
 * It is not safe to modify the @index_set during the enumeration.
 *
 * Since: 2.38
 */
void
g_index_set_enumerate_in_range (const GIndexSet         *index_set,
                                const GRange            *range,
                                GIndexSetEnumerateFlags  flags,
                                GIndexSetEnumerateFunc   func,
                                gpointer                 data)
{
  gboolean is_reverse;
  guint start, stop;
  guint i = 0;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (range != NULL);
  g_return_if_fail (func != NULL);

  if (index_set->ranges == NULL || index_set->ranges->len == 0)
    return;

  /* simple sanity check on the range */
  if (range->length == 0)
    return;

  is_reverse = (flags & G_INDEX_SET_ENUMERATE_REVERSE) != FALSE;

  start = g_index_set_get_position (index_set, range->location);
  if (start >= index_set->ranges->len)
    start = 0;

  stop = g_index_set_get_position (index_set, g_range_get_max (range) - 1);

#ifdef VERBOSE
  g_print ("Enumerating %u ranges %s using (%u - %u), start:%u, stop:%u\n",
           index_set->ranges->len,
           is_reverse ? "backwards" : "forwards",
           range->location, range->length,
           start, stop);
#endif

  i = start;
  while (is_reverse ? i >= stop : i < stop)
    {
      const GRange *r_range = g_index_set_get_range (index_set, i);
      guint r_start, r_stop;
      guint j = 0;

      if (is_reverse)
        {
          r_start = g_range_get_max (r_range) - 1;
          r_stop = r_range->location;
        }
      else
        {
          r_start = r_range->location;
          r_stop = g_range_get_max (r_range);
        }

#ifdef VERBOSE
      g_print ("Enumerating range %u of %u (%u - %u) %s, start:%u, stop:%u\n",
               i + 1, index_set->ranges->len,
               r_range->location, r_range->length,
               is_reverse ? "backwards" : "forwards",
               r_start, r_stop);
#endif

      j = r_start;
      while (is_reverse ? j >= r_stop : j < r_stop)
        {
          gboolean should_stop = func (j, data);

          if (should_stop)
            return;

          if (is_reverse)
            {
              if (j == 0)
                break;
              else
                j -= 1;
            }
          else
            j += 1;
        }

      if (is_reverse)
        {
          if (i == 0)
            break;
          else
            i -= 1;
        }
      else
        i += 1;
    }
}

/**
 * g_index_set_enumerate:
 * @index_set: a #GIndexSet
 * @flags: flags controlling the enumeration
 * @func: (scope call): a function to be called on every element of the set
 * @data: (closure): data to be passed to @func
 *
 * Calls @func on every element inside the @index_set. The function should
 * return %FALSE to continue the enumeration, or %TRUE to stop it.
 *
 * The @flags can be used to control the direction of the enumeration.
 *
 * It is not safe to modify the @index_set during the enumeration.
 *
 * Since: 2.38
 */
void
g_index_set_enumerate (const GIndexSet         *index_set,
                       GIndexSetEnumerateFlags  flags,
                       GIndexSetEnumerateFunc   func,
                       gpointer                 data)
{
  GRange range;
  guint first, last;

  g_return_if_fail (index_set != NULL);
  g_return_if_fail (func != NULL);

  first = g_index_set_get_first_index (index_set);
  if (first == G_INDEX_SET_NOT_FOUND)
    return;

  last = g_index_set_get_last_index (index_set);
  g_range_init (&range, first, (last - first) + 1);

  g_index_set_enumerate_in_range (index_set, &range, flags, func, data);
}
