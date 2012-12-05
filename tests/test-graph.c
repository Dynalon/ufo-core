#include <glib.h>
#include "test-suite.h"
#include "ufo-graph.h"
#include "ufo-node.h"


typedef struct {
    UfoGraph *graph;
    UfoGraph *sequence;
    UfoNode *root;
    UfoNode *target1;
    UfoNode *target2;
} Fixture;

static gpointer FOO_LABEL = GINT_TO_POINTER (0xDEADF00D);
static gpointer BAR_LABEL = GINT_TO_POINTER (0xF00BA);
static gpointer BAZ_LABEL = GINT_TO_POINTER (0xBA22BA22);

static void
fixture_setup (Fixture *fixture, gconstpointer data)
{
    fixture->graph = ufo_graph_new ();
    g_assert (UFO_IS_GRAPH (fixture->graph));

    fixture->sequence = ufo_graph_new ();

    ufo_graph_register_node_type (fixture->graph, UFO_TYPE_NODE);
    ufo_graph_register_node_type (fixture->sequence, UFO_TYPE_NODE);

    fixture->root = ufo_node_new (FOO_LABEL);
    fixture->target1 = ufo_node_new (BAR_LABEL);
    fixture->target2 = ufo_node_new (BAZ_LABEL);

    ufo_graph_connect_nodes (fixture->graph,
                             fixture->root,
                             fixture->target1,
                             FOO_LABEL);

    ufo_graph_connect_nodes (fixture->graph,
                             fixture->root,
                             fixture->target2,
                             BAR_LABEL);

    ufo_graph_connect_nodes (fixture->sequence,
                             fixture->root,
                             fixture->target1,
                             BAR_LABEL);

    ufo_graph_connect_nodes (fixture->sequence,
                             fixture->target1,
                             fixture->target2,
                             FOO_LABEL);
}

static void
fixture_teardown (Fixture *fixture, gconstpointer data)
{
    g_object_unref (fixture->graph);
    g_object_unref (fixture->sequence);
}

static void
test_connected (Fixture *fixture, gconstpointer data)
{
    g_assert (ufo_graph_is_connected (fixture->sequence,
                                      fixture->root,
                                      fixture->target1));
    g_assert (ufo_graph_is_connected (fixture->sequence,
                                      fixture->target1,
                                      fixture->target2));
    g_assert (!ufo_graph_is_connected (fixture->sequence,
                                       fixture->root,
                                       fixture->target2));
    g_assert (!ufo_graph_is_connected (fixture->sequence,
                                       fixture->target1,
                                       fixture->root));
    g_assert (!ufo_graph_is_connected (fixture->sequence,
                                       fixture->target2,
                                       fixture->root));
    g_assert (!ufo_graph_is_connected (fixture->sequence,
                                       fixture->target2,
                                       fixture->target1));
}

static void
test_get_roots (Fixture *fixture, gconstpointer data)
{
    GList *roots;

    roots = ufo_graph_get_roots (fixture->graph);
    g_assert (g_list_length (roots) == 1);
    g_assert (g_list_nth_data (roots, 0) == fixture->root);
    g_list_free (roots);
}

static void
test_get_num_nodes (Fixture *fixture, gconstpointer data)
{
    g_assert (ufo_graph_get_num_nodes (fixture->graph) == 3);
    g_assert (ufo_graph_get_num_nodes (fixture->sequence) == 3);
}

static void
test_get_num_edges (Fixture *fixture, gconstpointer data)
{
    g_assert (ufo_graph_get_num_edges (fixture->graph) == 2);
    g_assert (ufo_graph_get_num_edges (fixture->sequence) == 2);
}

static void
test_remove_edge (Fixture *fixture, gconstpointer data)
{
    GList *successors;

    successors = ufo_graph_get_successors (fixture->sequence, fixture->target1);
    g_assert (g_list_length (successors) == 1);
    g_list_free (successors);

    ufo_graph_remove_edge (fixture->sequence, fixture->target1, fixture->target2);
    successors = ufo_graph_get_successors (fixture->sequence, fixture->target1);
    g_assert (successors == NULL);
    g_assert (g_list_length (successors) == 0);
    g_list_free (successors);
    g_assert (ufo_graph_get_num_edges (fixture->sequence) == 1);
}

static void
test_get_labels (Fixture *fixture, gconstpointer data)
{
    g_assert (ufo_graph_get_edge_label (fixture->graph, fixture->root, fixture->target1) == FOO_LABEL);
    g_assert (ufo_graph_get_edge_label (fixture->graph, fixture->root, fixture->target2) == BAR_LABEL);
}

static void
test_split_and_join (Fixture *fixture, gconstpointer data)
{
    GList *successors;
    UfoNode *node;
    GList *path = NULL;

    path = g_list_append (path, fixture->root);
    path = g_list_append (path, fixture->target1);
    path = g_list_append (path, fixture->target2);

    ufo_graph_split (fixture->sequence, path);
    g_list_free (path);

    successors = ufo_graph_get_successors (fixture->sequence, fixture->root);
    g_assert (g_list_length (successors) == 2);

    node = UFO_NODE (g_list_nth_data (successors, 0));
    g_list_free (successors);

    successors = ufo_graph_get_successors (fixture->sequence, node);
    g_assert (g_list_length (successors) == 1);
    node = UFO_NODE (g_list_nth_data (successors, 0));
    g_list_free (successors);

    g_assert (ufo_node_equal (node, fixture->target2));
}

static gboolean
always_true (UfoNode *node)
{
    return TRUE;
}

static void
test_get_nodes_filtered (Fixture *fixture, gconstpointer data)
{
    GList *nodes;

    nodes = ufo_graph_get_nodes_filtered (fixture->sequence, always_true);
    g_assert (g_list_length (nodes) == 3);
    g_assert (g_list_find (nodes, fixture->root) != NULL);
    g_assert (g_list_find (nodes, fixture->target1) != NULL);
    g_assert (g_list_find (nodes, fixture->target2) != NULL);
    g_list_free (nodes);
}

void
test_add_graph (void)
{
    g_test_add ("/graph/connected",
                Fixture, NULL,
                fixture_setup, test_connected, fixture_teardown);

    g_test_add ("/graph/roots",
                Fixture, NULL,
                fixture_setup, test_get_roots, fixture_teardown);

    g_test_add ("/graph/num-nodes",
                Fixture, NULL,
                fixture_setup, test_get_num_nodes, fixture_teardown);

    g_test_add ("/graph/num-edges",
                Fixture, NULL,
                fixture_setup, test_get_num_edges, fixture_teardown);

    g_test_add ("/graph/remove-edge",
                Fixture, NULL,
                fixture_setup, test_remove_edge, fixture_teardown);

    g_test_add ("/graph/labels",
                Fixture, NULL,
                fixture_setup, test_get_labels, fixture_teardown);

    g_test_add ("/graph/split-join",
                Fixture, NULL,
                fixture_setup, test_split_and_join, fixture_teardown);

    g_test_add ("/graph/get-nodes-filtered",
                Fixture, NULL,
                fixture_setup, test_get_nodes_filtered, fixture_teardown);
}
