#include <glib-object.h>

typedef struct {
  GObject parent_instance;
} PropertyTest;

typedef struct {
  GObjectClass parent_class;
} PropertyTestClass;

typedef struct {
  int int_read_write;
  int int_read_only;
  int int_write_only;
} PropertyTestPrivate;

enum {
  PROP_INT_READ_WRITE = 1,
  PROP_INT_READ_ONLY,
  PROP_INT_WRITE_ONLY,

  LAST_PROPERTY
};

static GParamSpec *PropertyTest_props[LAST_PROPERTY] = { NULL, };

GType property_test_get_type (void);

G_DEFINE_TYPE_WITH_PRIVATE (PropertyTest, property_test, G_TYPE_OBJECT)

static void
property_test_class_init (PropertyTestClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  PropertyTest_props[PROP_INT_READ_WRITE] =
    g_int_property_new ("int-read-write",
                        G_PROPERTY_FLAGS_READWRITE,
                        G_PRIVATE_OFFSET (PropertyTest, int_read_write),
                        NULL, NULL);

  PropertyTest_props[PROP_INT_READ_ONLY] =
    g_int_property_new ("int-read-only",
                        G_PROPERTY_FLAGS_READABLE,
                        G_PRIVATE_OFFSET (PropertyTest, int_read_only),
                        NULL, NULL);

  PropertyTest_props[PROP_INT_WRITE_ONLY] =
    g_int_property_new ("int-write-only",
                        G_PROPERTY_FLAGS_WRITABLE,
                        G_PRIVATE_OFFSET (PropertyTest, int_write_only),
                        NULL, NULL);

  g_object_class_install_properties (gobject_class,
                                     LAST_PROPERTY,
                                     PropertyTest_props);
}

static void
property_test_init (PropertyTest *self)
{
}

static void
gproperty_flags_read_write (void)
{
  PropertyTest *t = g_object_new (property_test_get_type (), NULL);
  int int_foo = 0;

  g_object_set (t, "int-read-write", 42, NULL);
  g_object_get (t, "int-read-write", &int_foo, NULL);
  g_assert_cmpint (int_foo, ==, 42);

  g_object_unref (t);
}

static void
gproperty_flags_read_only (void)
{
  if (g_test_subprocess ())
    {
      PropertyTest *t = g_object_new (property_test_get_type (), NULL);

      g_object_set (t, "int-read-only", 42, NULL);

      g_object_unref (t);
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*property 'int-read-only' of object class 'PropertyTest' is not writable*");
}

static void
gproperty_flags_write_only (void)
{
  if (g_test_subprocess ())
    {
      PropertyTest *t = g_object_new (property_test_get_type (), NULL);
      int int_foo = 0;

      g_object_get (t, "int-write-only", &int_foo, NULL);
      g_assert_cmpint (int_foo, ==, 0);

      g_object_unref (t);
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*property 'int-write-only' of object class 'PropertyTest' is not readable*");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gproperty/flags/read-only", gproperty_flags_read_only);
  g_test_add_func ("/gproperty/flags/write-only", gproperty_flags_write_only);
  g_test_add_func ("/gproperty/flags/read-write", gproperty_flags_read_write);

  return g_test_run ();
}
