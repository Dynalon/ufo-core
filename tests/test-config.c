/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib-object.h>
#include "test-suite.h"
#include "ufo-config.h"

static void
test_path (void)
{
    UfoConfig *config;
    GObject     *object;
    GValueArray *array;
    GValue       value = {0};
    const gchar *p1 = "/usr/foo/bar";
    const gchar *p2 = "/home/user/foo";
    GList *paths;

    /* Initialize value array with the two static strings */
    array = g_value_array_new (2);
    g_value_init (&value, G_TYPE_STRING);
    g_value_set_string (&value, p1);
    g_value_array_append (array, &value);

    g_value_reset (&value);
    g_value_set_string (&value, p2);
    g_value_array_append (array, &value);

    /* Create a new object object and see what happens */
    object = g_object_new (UFO_TYPE_CONFIG, "paths", array, NULL);

    g_value_array_free (array);

    g_object_get (object,
                  "paths", &array,
                  NULL);

    g_assert (array->n_values == 2);

    /* Check that paths match the ones we added */
    g_assert (g_strcmp0 (p1, g_value_get_string (g_value_array_get_nth (array, 0))) == 0);
    g_assert (g_strcmp0 (p2, g_value_get_string (g_value_array_get_nth (array, 1))) == 0);

    g_value_array_free (array);

    /* Now check the C API */
    config = UFO_CONFIG (object);
    paths = ufo_config_get_paths (config);

    g_assert (paths != NULL);
    g_assert (g_list_length (paths) >= 2);
    g_assert (g_strcmp0 (p1, g_list_nth_data (paths, 0)) == 0);
    g_assert (g_strcmp0 (p2, g_list_nth_data (paths, 1)) == 0);

    g_list_free (paths);
    g_object_unref (object);
}

static void
test_path_not_set (void)
{
    UfoConfig *config;
    GList *paths;

    config = ufo_config_new ();
    paths = ufo_config_get_paths (config);
    g_assert (paths != NULL);
    g_list_free (paths);
    g_object_unref (config);
}

void
test_add_config (void)
{
    g_test_add_func("/config/path",
                    test_path);

    /* Check trac ticket #127 */
    g_test_add_func("/config/path-not-set",
                    test_path_not_set);
}
