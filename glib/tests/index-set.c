#include <glib.h>

static void
index_set_init (void)
{
  GIndexSet *set;

  if (g_test_verbose ())
    g_print ("Init + alloc\n");
  set = g_index_set_init (g_index_set_alloc ());
  g_assert (set != NULL);
  g_assert (g_index_set_is_mutable (set));
  g_assert_cmpint (g_index_set_get_size (set), ==, 0);
  g_assert_cmpint (g_index_set_get_first_index (set), ==, G_INDEX_SET_NOT_FOUND);
  g_assert_cmpint (g_index_set_get_last_index (set), ==, G_INDEX_SET_NOT_FOUND);

  if (g_test_verbose ())
    g_print ("Init empty\n");
  set = g_index_set_init_empty (g_index_set_alloc ());
  g_assert (set != NULL);
  g_assert (!g_index_set_is_mutable (set));
  g_assert_cmpint (g_index_set_get_size (set), ==, 0);
  g_assert_cmpint (g_index_set_get_first_index (set), ==, G_INDEX_SET_NOT_FOUND);
  g_assert_cmpint (g_index_set_get_last_index (set), ==, G_INDEX_SET_NOT_FOUND);

  if (g_test_verbose ())
    g_print ("Init with index [ 0 ]\n");
  set = g_index_set_init_with_index (set, 0);
  g_assert (!g_index_set_is_mutable (set));
  g_assert_cmpint (g_index_set_get_size (set), ==, 1);
  g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
  g_assert_cmpint (g_index_set_get_last_index (set), ==, 0);

  {
    GRange range = G_RANGE_INIT (0, 10);

    if (g_test_verbose ())
      g_print ("Init with range [ 0, 10 ]\n");

    set = g_index_set_init_with_range (set, &range);
    g_assert (set != NULL);
    g_assert (!g_index_set_is_mutable (set));
    g_assert_cmpint (g_index_set_get_size (set), ==, 10);
    g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
    g_assert_cmpint (g_index_set_get_last_index (set), ==, 9);
  }

  {
    const guint even_indices[] = { 0, 2, 4, 6, 8 };
    const guint odd_indices[] = { 1, 3, 5, 7, 9 };
    const guint unsorted_indices[] = { 4, 2, 8, 0, 6 };
    const guint unsorted_dup_indices[] = { 4, 8, 2, 8, 0, 2, 6 };

    if (g_test_verbose ())
      g_print ("Init with even indices: [ 0, 2, 4, 6, 8 ]\n");
    set = g_index_set_init_with_indicesv (set,
                                          G_N_ELEMENTS (even_indices),
                                          even_indices);
    g_assert (set != NULL);
    g_assert (!g_index_set_is_mutable (set));
    g_assert_cmpint (g_index_set_get_size (set), ==, G_N_ELEMENTS (even_indices));
    g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
    g_assert_cmpint (g_index_set_get_last_index (set), ==, 8);

    if (g_test_verbose ())
      g_print ("Init with odd indices: [ 1, 3, 5, 7, 9 ]\n");
    set = g_index_set_init_with_indicesv (set,
                                          G_N_ELEMENTS (odd_indices),
                                          odd_indices);
    g_assert (set != NULL);
    g_assert (!g_index_set_is_mutable (set));
    g_assert_cmpint (g_index_set_get_size (set), ==, G_N_ELEMENTS (odd_indices));
    g_assert_cmpint (g_index_set_get_first_index (set), ==, 1);
    g_assert_cmpint (g_index_set_get_last_index (set), ==, 9);

    if (g_test_verbose ())
      g_print ("Init with unsorted indices: [ 4, 2, 8, 0, 6 ]\n");
    set = g_index_set_init_with_indicesv (set,
                                          G_N_ELEMENTS (unsorted_indices),
                                          unsorted_indices);
    g_assert (set != NULL);
    g_assert (!g_index_set_is_mutable (set));
    g_assert_cmpint (g_index_set_get_size (set), ==, G_N_ELEMENTS (unsorted_indices));
    g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
    g_assert_cmpint (g_index_set_get_last_index (set), ==, 8);

    if (g_test_verbose ())
      g_print ("Init with unsorted, duplicate indices: [ 4, 8, 2, 8, 0, 2, 6 ]\n");
    set = g_index_set_init_with_indicesv (set,
                                          G_N_ELEMENTS (unsorted_dup_indices),
                                          unsorted_dup_indices);
    g_assert (set != NULL);
    g_assert (!g_index_set_is_mutable (set));
    g_assert_cmpint (g_index_set_get_size (set), <, G_N_ELEMENTS (unsorted_dup_indices));
    g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
    g_assert_cmpint (g_index_set_get_last_index (set), ==, 8);
  }
}

static void
index_set_contains (void)
{
  GIndexSet *set;
  GRange range = G_RANGE_INIT (0, 10);
  guint i;

  set = g_index_set_init_with_range (g_index_set_alloc (), &range);
  g_assert_cmpint (g_index_set_get_size (set), ==, range.length);

  if (g_test_verbose ())
    g_print ("Range [ 0, 10 ]: [");
  for (i = range.location; i < range.length; i++)
    {
      if (g_test_verbose ())
        g_print (" %u", i);
      g_assert (g_index_set_contains_index (set, i));
    }
  if (g_test_verbose ())
    g_print (" ]\n");

  g_range_init (&range, 0, 5);
  if (g_test_verbose ())
    g_print ("Range [ 0, 5 ]\n");
  g_assert (g_index_set_contains_range (set, &range));

  g_range_init (&range, 5, 5);
  if (g_test_verbose ())
    g_print ("Range [ 5, 5 ]\n");
  g_assert (g_index_set_contains_range (set, &range));

  g_range_init (&range, 0, 10);
  if (g_test_verbose ())
    g_print ("Range [ 0, 10 ]\n");
  g_assert (g_index_set_contains_range (set, &range));

  g_range_init (&range, 0, 11);
  if (g_test_verbose ())
    g_print ("Range [ 0, 11 ]\n");
  g_assert (!g_index_set_contains_range (set, &range));

  g_index_set_unref (set);
}

static void
index_set_add (void)
{
  GIndexSet *set = g_index_set_alloc ();

  g_index_set_init (set);
  g_assert (g_index_set_is_mutable (set));

  if (g_test_verbose ())
    g_print ("Add [ 5 ]\n");
  g_index_set_add_index (set, 5);
  g_assert_cmpint (g_index_set_get_size (set), ==, 1);
  g_assert_cmpint (g_index_set_get_first_index (set), ==, 5);
  g_assert_cmpint (g_index_set_get_first_index (set),
                   ==,
                   g_index_set_get_last_index (set));

  if (g_test_verbose ())
    g_print ("Add [ 0, 1, 2 ]\n");
  g_index_set_add_indices (set, 3, 0, 1, 2);
  g_assert_cmpint (g_index_set_get_size (set), ==, 4);
  g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
  g_assert_cmpint (g_index_set_get_last_index (set), ==, 5);
  g_assert_cmpint (g_index_set_get_first_index (set),
                   !=,
                   g_index_set_get_last_index (set));

  if (g_test_verbose ())
    g_print ("Add [ 7, 6, 2 ]\n");
  g_index_set_add_indices (set, 3, 7, 6, 2);
  g_assert_cmpint (g_index_set_get_size (set), ==, 6);
  g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
  g_assert_cmpint (g_index_set_get_last_index (set), ==, 7);

  {
    GRange range = G_RANGE_INIT (0, 10);

    if (g_test_verbose ())
      g_print ("Add [ 0, 10 ]\n");
    g_index_set_add_range (set, &range);
    g_assert_cmpint (g_index_set_get_size (set), ==, 10);
    g_assert_cmpint (g_index_set_get_first_index (set), ==, 0);
    g_assert_cmpint (g_index_set_get_last_index (set), ==, 9);
  }

  g_index_set_make_immutable (set);
  g_assert (!g_index_set_is_mutable (set));

  g_index_set_unref (set);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/index-set/init", index_set_init);
  g_test_add_func ("/index-set/contains", index_set_contains);
  g_test_add_func ("/index-set/add", index_set_add);

  return g_test_run();
}
