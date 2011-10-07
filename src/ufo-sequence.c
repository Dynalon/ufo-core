#include "ufo-sequence.h"
#include "ufo-element.h"

struct _UfoSequencePrivate {
    GList *children;

    /* XXX: In fact we don't need those two queues, because the input of a
     * sequence corresponds to the input of the very first child and the output
     * with the output of the very last child. So, in the future we might
     * respect this fact and drop these queues. */
    GAsyncQueue *input_queue;
    GAsyncQueue *output_queue;
    cl_command_queue command_queue;
};

static void ufo_element_iface_init(UfoElementInterface *iface);

G_DEFINE_TYPE_WITH_CODE(UfoSequence, 
                        ufo_sequence, 
                        UFO_TYPE_CONTAINER,
                        G_IMPLEMENT_INTERFACE(UFO_TYPE_ELEMENT,
                                              ufo_element_iface_init));

#define UFO_SEQUENCE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_SEQUENCE, UfoSequencePrivate))


/* 
 * Public Interface
 */
/**
 * \brief Create a new UfoSequence object
 * \public \memberof UfoSequence
 * \return A UfoElement
 */
UfoSequence *ufo_sequence_new()
{
    return UFO_SEQUENCE(g_object_new(UFO_TYPE_SEQUENCE, NULL));
}

/* 
 * Virtual Methods 
 */
static void ufo_sequence_dispose(GObject *object)
{
    UfoSequence *self = UFO_SEQUENCE(object);

    /* FIXME: we are leaking objects due to Ethos */
    g_list_foreach(self->priv->children, (GFunc) g_object_unref, NULL);

    if (self->priv->input_queue != NULL)
        g_async_queue_unref(self->priv->input_queue);
    if (self->priv->output_queue != NULL)
        g_async_queue_unref(self->priv->output_queue);

    G_OBJECT_CLASS(ufo_sequence_parent_class)->dispose(object);
}

static void ufo_sequence_finalize(GObject *object)
{
    UfoSequence *self = UFO_SEQUENCE(object);
    g_list_free(self->priv->children);
    G_OBJECT_CLASS(ufo_sequence_parent_class)->finalize(object);
}

static void ufo_sequence_add_element(UfoContainer *container, UfoElement *child)
{
    if (container == NULL || child == NULL)
        return;

    /* In this method we also need to add the asynchronous queues. It is
     * important to understand the two cases:
     * 
     * 1. There is no element in the list. Then we just add the new element
     * with new input_queue as input_queue from container and no output.
     *
     * 2. There is an element in the list. Then we try to get that old element's
     * output queue.
     */
    UfoSequence *self = UFO_SEQUENCE(container);
    GList *last = g_list_last(self->priv->children);
    GAsyncQueue *prev = NULL;

    GAsyncQueue *queue = ufo_element_get_input_queue(child);
    if (queue != NULL && last != NULL) {
        UfoElement *last_element = UFO_ELEMENT(last->data);
        ufo_element_set_output_queue(last_element, queue);
    }

    /*
    if (last != NULL) {
        UfoElement *last_element = UFO_ELEMENT(last->data);
        prev = ufo_element_get_output_queue(last_element);
    }
    else {
        prev = self->priv->input_queue;
    }
    */


    /* Ok, we have some old output and connect it to the newly added element */
    /*ufo_element_set_input_queue(child, prev);*/

    /* Now, we create a new output that is also going to be the container's
     * real output */
    /*GAsyncQueue *next = g_async_queue_new();*/
    /*ufo_element_set_output_queue(child, next);*/
    /*ufo_element_set_command_queue(child, self->priv->command_queue);*/
    /*self->priv->output_queue = next;*/
    /*self->priv->children = g_list_append(self->priv->children, child);*/
    g_object_ref(child);
}

/**
 * ufo_sequence_get_elements:
 * 
 * Return value: (element-type UfoElement) (transfer none): list of children.
 */
static GList *ufo_sequence_get_elements(UfoContainer *container)
{
    UfoSequence *self = UFO_SEQUENCE(container);
    return self->priv->children;
}

static gpointer ufo_sequence_process_thread(gpointer data)
{
    ufo_element_process(UFO_ELEMENT(data));
    return NULL;
}

static void ufo_sequence_process(UfoElement *element)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    GList *threads = NULL;
    GError *error = NULL;
    for (guint i = 0; i < g_list_length(self->priv->children); i++) {
        UfoElement *child = UFO_ELEMENT(g_list_nth_data(self->priv->children, i));
        threads = g_list_append(threads,
                g_thread_create(ufo_sequence_process_thread, child, TRUE, &error));
    }
    g_list_foreach(threads, ufo_container_join_threads, NULL);
    g_list_free(threads);
}

static void ufo_sequence_set_input_queue(UfoElement *element, GAsyncQueue *queue)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    if (queue)
        g_async_queue_ref(queue);
    self->priv->input_queue = queue;
}

static void ufo_sequence_set_output_queue(UfoElement *element, GAsyncQueue *queue)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    if (queue)
        g_async_queue_ref(queue);
    self->priv->output_queue = queue;
}

static GAsyncQueue *ufo_sequence_get_input_queue(UfoElement *element)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    return self->priv->input_queue;
}

static GAsyncQueue *ufo_sequence_get_output_queue(UfoElement *element)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    return self->priv->input_queue;
}

static void ufo_sequence_set_command_queue(UfoElement *element, gpointer command_queue)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    self->priv->command_queue = command_queue;
}

static gpointer ufo_sequence_get_command_queue(UfoElement *element)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    return self->priv->command_queue;
}

static float ufo_sequence_get_time_spent(UfoElement *element)
{
    UfoSequence *self = UFO_SEQUENCE(element);
    float time_spent = 0.0f;
    for (guint i = 0; i < g_list_length(self->priv->children); i++) {
        UfoElement *child = UFO_ELEMENT(g_list_nth_data(self->priv->children, i));
        time_spent += ufo_element_get_time_spent(child);
    }
    return time_spent;
}

/*
 * Type/Class Initialization
 */
static void ufo_element_iface_init(UfoElementInterface *iface)
{
    iface->process = ufo_sequence_process;
    iface->set_input_queue = ufo_sequence_set_input_queue;
    iface->set_output_queue = ufo_sequence_set_output_queue;
    iface->set_command_queue = ufo_sequence_set_command_queue;
    iface->get_input_queue = ufo_sequence_get_input_queue;
    iface->get_output_queue = ufo_sequence_get_output_queue;
    iface->get_command_queue = ufo_sequence_get_command_queue;
    iface->get_time_spent = ufo_sequence_get_time_spent;
}

static void ufo_sequence_class_init(UfoSequenceClass *klass)
{
    /* override methods */
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    UfoContainerClass *container_class = UFO_CONTAINER_CLASS(klass);

    gobject_class->dispose = ufo_sequence_dispose;
    gobject_class->finalize = ufo_sequence_finalize;
    container_class->add_element = ufo_sequence_add_element;
    container_class->get_elements = ufo_sequence_get_elements;

    /* install private data */
    g_type_class_add_private(klass, sizeof(UfoSequencePrivate));
}

static void ufo_sequence_init(UfoSequence *self)
{
    self->priv = UFO_SEQUENCE_GET_PRIVATE(self);
    self->priv->children = NULL;
    self->priv->input_queue = NULL;
    self->priv->output_queue = NULL;
    self->priv->command_queue = NULL;
}
