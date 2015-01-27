/* gref.h: Reference counted memory areas
 *
 * Copyright © 2015  Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:refcount
 * @short_description: reference counted memory areas
 * @title: References
 *
 * These functions provide support for allocating and freeing reference
 * counted memory areas.
 *
 * Reference counted memory areas are kept alive as long as something holds
 * a reference on them; as soon as their reference count drops to zero, the
 * associated memory is freed.
 */

#include "config.h"
#include "glibconfig.h"

#include "gref.h"

#include "gmessages.h"
#include "gtestutils.h"
#include "valgrind.h"

#include <string.h>

#define STRUCT_ALIGNMENT        (2 * sizeof (gsize))
#define ALIGN_STRUCT(offset)    ((offset + (STRUCT_ALIGNMENT - 1)) & -STRUCT_ALIGNMENT)

#define G_REF_SIZE              sizeof (GRef)
#define G_REF(ptr)              (GRef *) (((gchar *) (ptr)) - G_REF_SIZE)

typedef struct _GRef    GRef;

struct _GRef
{
  volatile int ref_count;

  gsize alloc_size;

  GDestroyNotify notify;
};

#ifdef G_ENABLE_DEBUG
#include "ghash.h"
#include "gthread.h"

static GHashTable *referenced_pointers;
G_LOCK_DEFINE_STATIC (referenced_pointers);

static inline void
g_ref_register (gpointer ref)
{
  G_LOCK (referenced_pointers);
  if (G_UNLIKELY (referenced_pointers == NULL))
    referenced_pointers = g_hash_table_new (NULL, NULL);

  g_hash_table_add (referenced_pointers, ref);
  G_UNLOCK (referenced_pointers);
}

static inline void
g_ref_unregister (gpointer ref)
{
  G_LOCK (referenced_pointers);
  if (G_LIKELY (referenced_pointers != NULL))
    g_hash_table_remove (referenced_pointers, ref);
  G_UNLOCK (referenced_pointers);
}

static gboolean
g_is_ref (gpointer ref)
{
  gboolean res = FALSE;

  G_LOCK (referenced_pointers);
  if (G_LIKELY (referenced_pointers != NULL))
    res = g_hash_table_contains (referenced_pointers, ref);
  G_UNLOCK (referenced_pointers);

  return res;
}
#endif

/**
 * g_ref_destroy:
 * @ref: a reference counted memory area
 *
 * Forces the destruction of a reference counter memory area.
 *
 * Since: 2.44
 */
void
g_ref_destroy (gpointer ref)
{
  GRef *real = G_REF (ref);
  gsize alloc_size = real->alloc_size;
  gsize private_size = G_REF_SIZE;
  gchar *allocated = ((gchar *) ref) - private_size;

  if (real->notify != NULL)
    real->notify (ref);

#ifdef G_ENABLE_DEBUG
  g_ref_unregister (ref);
#endif

  if (RUNNING_ON_VALGRIND)
    {
      private_size += ALIGN_STRUCT (1);
      allocated -= ALIGN_STRUCT (1);

      *(gpointer *) (allocated + private_size + alloc_size) = NULL;
      g_free (allocated);
    }
  else
    g_free (allocated);
}

static gpointer
g_ref_alloc_internal (gsize          alloc_size,
                      gboolean       atomic,
                      gboolean       clear,
                      GDestroyNotify notify)
{
  gsize private_size = G_REF_SIZE;
  gchar *allocated;
  GRef *real;

  g_return_val_if_fail (alloc_size != 0, NULL);

  if (RUNNING_ON_VALGRIND)
    {
      private_size += ALIGN_STRUCT (1);

      if (clear)
        allocated = g_malloc0 (private_size + alloc_size + sizeof (gpointer));
      else
        allocated = g_malloc (private_size + alloc_size + sizeof (gpointer));

      *(gpointer *) (allocated + private_size + alloc_size) = allocated + ALIGN_STRUCT (1);

      VALGRIND_MALLOCLIKE_BLOCK (allocated + private_size, alloc_size + sizeof (gpointer), 0, TRUE);
      VALGRIND_MALLOCLIKE_BLOCK (allocated + ALIGN_STRUCT (1), private_size - ALIGN_STRUCT (1), 0, TRUE);
    }
  else
    {
      if (clear)
        allocated = g_malloc0 (private_size + alloc_size);
      else
        allocated = g_malloc (private_size + alloc_size);
    }

#ifdef G_ENABLE_DEBUG
  g_ref_register (allocated + private_size);
#endif

  real = (GRef *) allocated;
  real->ref_count = atomic ? -1 : 1;
  real->notify = notify;
  real->alloc_size = alloc_size;

  return allocated + private_size;
}

/**
 * g_ref_alloc:
 * @alloc_size: the number of bytes to allocate
 * @notify: (nullable): a function to be called when the reference
 *   count drops to zero
 *
 * Allocates a reference counted memory area.
 *
 * Reference counted memory areas are automatically freed when their
 * reference count drops to zero.
 *
 * Use g_ref_acquire() to acquire a reference, and g_ref_release()
 * to release it.
 *
 * The contents of the returned memory are undefined.
 *
 * Returns: a pointer to the allocated memory
 *
 * Since: 2.44
 */
gpointer
g_ref_alloc (gsize          alloc_size,
             GDestroyNotify notify)
{
  return g_ref_alloc_internal (alloc_size, FALSE, FALSE, notify);
}

/**
 * g_ref_alloc0:
 * @alloc_size: the number of bytes to allocate
 * @notify: (nullable): a function to be called when the reference
 *   count drops to zero
 *
 * Allocates a reference counted memory area.
 *
 * Reference counted memory areas are automatically freed when their
 * reference count drops to zero.
 *
 * Use g_ref_acquire() to acquire a reference, and g_ref_release()
 * to release it.
 *
 * The contents of the returned memory are set to 0.
 *
 * Returns: a pointer to the allocated memory
 *
 * Since: 2.44
 */
gpointer
g_ref_alloc0 (gsize          alloc_size,
              GDestroyNotify notify)
{
  return g_ref_alloc_internal (alloc_size, FALSE, TRUE, notify);
}

/**
 * g_atomic_ref_alloc:
 * @alloc_size: the number of bytes to allocate
 * @notify: (nullable): a function to be called when the reference
 *   count drops to zero
 *
 * Allocates a atomically reference counted memory area.
 *
 * Reference counted memory areas are automatically freed when their
 * reference count drops to zero.
 *
 * Use g_ref_acquire() to acquire a reference, and g_ref_release()
 * to release it. References are acquired and released using the
 * atomic primitives available on the system.
 *
 * The contents of the returned memory are undefined.
 *
 * Returns: a pointer to the allocated memory
 *
 * Since: 2.44
 */
gpointer
g_atomic_ref_alloc (gsize          alloc_size,
                    GDestroyNotify notify)
{
  return g_ref_alloc_internal (alloc_size, TRUE, FALSE, notify);
}

/**
 * g_atomic_ref_alloc0:
 * @alloc_size: the number of bytes to allocate
 * @notify: (nullable): a function to be called when the reference
 *   count drops to zero
 *
 * Allocates a atomically reference counted memory area.
 *
 * Reference counted memory areas are automatically freed when their
 * reference count drops to zero.
 *
 * Use g_ref_acquire() to acquire a reference, and g_ref_release()
 * to release it. References are acquired and released using the
 * atomic primitives available on the system.
 *
 * The contents of the returned memory are set to 0.
 *
 * Returns: a pointer to the allocated memory
 *
 * Since: 2.44
 */
gpointer
g_atomic_ref_alloc0 (gsize          alloc_size,
                     GDestroyNotify notify)
{
  return g_ref_alloc_internal (alloc_size, TRUE, TRUE, notify);
}

/**
 * g_ref_realloc:
 * @ref: a reference counted memory area to reallocate
 * @new_size: new size of the memory area
 *
 * Reallocates a reference counted memory area to @new_size.
 *
 * The reference count is kept at the same value.
 *
 * Returns: the newly reallocated memory
 *
 * Since: 2.44
 */
gpointer
g_ref_realloc (gpointer ref,
               gsize    new_size)
{
  GRef *real = G_REF (ref);
  gsize private_size = G_REF_SIZE;
  char *allocated;

  GDestroyNotify old_notify = real->notify;
  int old_ref_count = real->ref_count;

  if (RUNNING_ON_VALGRIND)
    {
      private_size += ALIGN_STRUCT (1);

      allocated = g_realloc (real, private_size + new_size + sizeof (gpointer));
      *(gpointer *) (allocated + private_size + new_size) = allocated + ALIGN_STRUCT (1);

      VALGRIND_MALLOCLIKE_BLOCK (allocated + private_size, new_size + sizeof (gpointer), 0, TRUE);
      VALGRIND_MALLOCLIKE_BLOCK (allocated + ALIGN_STRUCT (1), private_size - ALIGN_STRUCT (1), 0, TRUE);
    }
  else
    allocated = g_realloc (real, private_size + new_size);

  real = (GRef *) allocated;
  real->ref_count = old_ref_count;
  real->notify = old_notify;
  real->alloc_size = new_size;

  return allocated + private_size;
}

/**
 * g_ref_dup:
 * @ref: a reference counted memory area
 *
 * Duplicates existing data into a reference counted memory area.
 *
 * Returns: the newly allocated reference counted area
 *
 * Since: 2.44
 */
gpointer
g_ref_dup (gconstpointer  data,
           gsize          alloc_size,
           GDestroyNotify notify)
{
  gpointer res = g_ref_alloc (alloc_size, notify);

  memcpy (res, data, alloc_size);

  return res;
}

/**
 * g_ref_acquire:
 * @ref: a reference counted memory area
 *
 * Acquires a reference on the given memory area.
 *
 * Returns: the memory area, with its reference count increased by 1
 *
 * Since: 2.44
 */
gpointer
g_ref_acquire (gpointer ref)
{
  GRef *real = G_REF (ref);

  g_return_val_if_fail (ref != NULL, NULL);
#ifdef G_ENABLE_DEBUG
  g_return_val_if_fail (g_is_ref (ref), ref);
#endif

  if (g_atomic_int_get (&real->ref_count) < 0)
    g_atomic_int_add (&real->ref_count, -1);
  else
    real->ref_count += 1;

  return ref;
}

/**
 * g_ref_release:
 * @ref: a reference counted memory area
 *
 * Releases a reference acquired using g_ref_acquire().
 *
 * If the reference count drops to zero, the memory area is freed.
 *
 * Since: 2.44
 */
void
g_ref_release (gpointer ref)
{
  GRef *real = G_REF (ref);
  int ref_count;

  g_return_if_fail (ref != NULL);
#ifdef G_ENABLE_DEBUG
  g_return_if_fail (g_is_ref (ref));
#endif

again:
  ref_count = g_atomic_int_get (&real->ref_count);
  g_assert (ref_count != 0);

  if (ref_count == -1 || ref_count == 1)
    g_ref_destroy (ref);
  else if (ref_count > 0)
    real->ref_count -= 1;
  else if (G_UNLIKELY (!g_atomic_int_compare_and_exchange (&real->ref_count,
                                                           ref_count,
                                                           ref_count + 1)))
    goto again;
}

/**
 * g_ref_make_atomic:
 * @ref: a reference counted memory area
 *
 * Makes reference count operations on a reference counted memory area
 * always atomic.
 *
 * Since: 2.44
 */
void
g_ref_make_atomic (gpointer ref)
{
  GRef *real = G_REF (ref);
  int ref_count;

  g_return_if_fail (ref != NULL);
#ifdef G_ENABLE_DEBUG
  g_return_if_fail (g_is_ref (ref));
#endif

  ref_count = g_atomic_int_get (&real->ref_count);
  if (ref_count < 0)
    return;

  real->ref_count = -ref_count;
}

/**
 * g_string_ref_new:
 * @str: the string to reference
 *
 * Creates a new reference counted string.
 *
 * You can acquire a reference on the string using g_ref_acquire() and
 * release it using g_ref_release().
 *
 * The returned string can be used with any string utility function
 * transparently. Instead of copying the string, use the reference
 * counting API to acquire and release references when needed.
 *
 * Once the last reference on the string is released, the string will
 * be freed.
 *
 * Returns: a reference counted string
 *
 * Since: 2.44
 */
char *
g_string_ref_new (const char *str)
{
  gsize len = strlen (str);
  char *res = g_ref_alloc (len + 1, NULL);

  memcpy (res, str, len);
  res[len] = '\0';

  return res;
}
