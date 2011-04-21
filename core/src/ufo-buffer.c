#include <string.h>
#include <CL/cl.h>
#include "ufo-buffer.h"

G_DEFINE_TYPE(UfoBuffer, ufo_buffer, G_TYPE_OBJECT);

#define UFO_BUFFER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_BUFFER, UfoBufferPrivate))

#define UFO_BUFFER_ERROR ufo_buffer_error_quark()

enum UfoBufferError {
    UFO_BUFFER_ERROR_WRONG_SIZE
};

enum {
    PROP_0,
    PROP_FINISHED
};

typedef enum {
    NO_DATA,
    CPU_DATA_VALID,
    GPU_DATA_VALID
} datastates;

struct _UfoBufferPrivate {
    gint32      width;
    gint32      height;

    gboolean    finished;
    datastates  state;
    float       *cpu_data;
    cl_mem      gpu_data;
    cl_command_queue command_queue;
};

static void ufo_buffer_set_dimensions(UfoBuffer *self, gint32 width, gint32 height);

GQuark ufo_buffer_error_quark(void)
{
    return g_quark_from_static_string("ufo-buffer-error-quark");
}

/* 
 * Public Interface
 */

/**
 * \brief Create a new buffer with given dimensions
 *
 * \param[in] width Width of the two-dimensional buffer
 * \param[in] height Height of the two-dimensional buffer
 *
 * \return Buffer with allocated memory
 *
 * \note Filters should never allocate buffers on their own using this method
 * but use the UfoResourceManager method ufo_resource_manager_request_buffer().
 */
UfoBuffer *ufo_buffer_new(gint32 width, gint32 height)
{
    UfoBuffer *buffer = UFO_BUFFER(g_object_new(UFO_TYPE_BUFFER, NULL));
    ufo_buffer_set_dimensions(buffer, width, height);
    return buffer;
}


static void ufo_buffer_set_dimensions(UfoBuffer *self, gint32 width, gint32 height)
{
    g_return_if_fail(UFO_IS_BUFFER(self));
    /* FIXME: What to do when buffer already allocated memory? Re-size? */
    self->priv->width = width;
    self->priv->height = height;
}

void ufo_buffer_get_dimensions(UfoBuffer *self, gint32 *width, gint32 *height)
{
    g_return_if_fail(UFO_IS_BUFFER(self));
    *width = self->priv->width;
    *height = self->priv->height;
}

/**
 * \note This does not copy data from host to device.
 */
void ufo_buffer_create_gpu_buffer(UfoBuffer *self, cl_mem mem)
{
    self->priv->gpu_data = mem;
}

void ufo_buffer_set_cpu_data(UfoBuffer *self, float *data, gsize n, GError **error)
{
    const gsize num_bytes = self->priv->width * self->priv->height * sizeof(float);
    if ((n * sizeof(float)) > num_bytes) {
        g_set_error(error,
                UFO_BUFFER_ERROR,
                UFO_BUFFER_ERROR_WRONG_SIZE,
                "Trying to set more data than buffer dimensions allow");
        return;
    }
    if (self->priv->cpu_data == NULL)
        self->priv->cpu_data = g_malloc0(num_bytes);

    memcpy(self->priv->cpu_data, data, num_bytes);
    self->priv->state = CPU_DATA_VALID;
}

/**
 * \brief Spread raw data without increasing the contrast
 *
 * The fundamental data type of a UfoBuffer is one single-precision floating
 * point per pixel. To increase performance it is possible to load arbitrary
 * integer data with ufo_buffer_set_cpu_data() and convert that data with this
 * method.
 *
 * \param[in] self UfoBuffer object
 * \param[in] source_depth The original integer data type. This could be
 * UFO_BUFFER_DEPTH_8 for 8-bit data or UFO_BUFFER_DEPTH_16 for 16-bit data.
 */
void ufo_buffer_reinterpret(UfoBuffer *self, gint source_depth, gsize n)
{
    float *dst = self->priv->cpu_data;
    /* To save a memory allocation and several copies, we process data from back
     * to front. This is possible if src bit depth is at most half as wide as
     * the 32-bit target buffer. The processor cache should not be a problem. */
    if (source_depth == UFO_BUFFER_DEPTH_8) {
        guint8 *src = (guint8 *) self->priv->cpu_data;
        for (int index = (n-1); index >= 0; index--)
            dst[index] = src[index] / 255.0;
    }
    else if (source_depth == UFO_BUFFER_DEPTH_16) {
        guint16 *src = (guint16 *) self->priv->cpu_data;
        for (int index = (n-1); index >= 0; index--)
            dst[index] = src[index] / 65535.0;
    }
}

/*
 * \brief Get raw pixel data in a flat array (row-column format)
 *
 * \return Pointer to a character array of raw data bytes
 */
float* ufo_buffer_get_cpu_data(UfoBuffer *self)
{
    UfoBufferPrivate *priv = UFO_BUFFER_GET_PRIVATE(self);
    switch (priv->state) {
        case CPU_DATA_VALID:
            break;
        case GPU_DATA_VALID:
            /* TODO: download from gpu */
            priv->state = CPU_DATA_VALID;
            break;
        case NO_DATA:
            priv->cpu_data = g_malloc0(priv->width * priv->height * sizeof(float));
            priv->state = CPU_DATA_VALID;
            break;
    }

    return priv->cpu_data;
}

cl_mem ufo_buffer_get_gpu_data(UfoBuffer *self)
{
    UfoBufferPrivate *priv = UFO_BUFFER_GET_PRIVATE(self);
    switch (priv->state) {
        case CPU_DATA_VALID:
            /* TODO: upload to gpu */
            priv->state = GPU_DATA_VALID;
            break;
        case GPU_DATA_VALID:
            break;
        case NO_DATA:
            return NULL;
    }
    return priv->gpu_data;
}

/* 
 * Virtual Methods 
 */
static void ufo_filter_set_property(GObject *object,
    guint           property_id,
    const GValue    *value,
    GParamSpec      *pspec)
{
    UfoBuffer *self = UFO_BUFFER(object);

    switch (property_id) {
        case PROP_FINISHED:
            self->priv->finished = g_value_get_boolean(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void ufo_filter_get_property(GObject *object,
    guint       property_id,
    GValue      *value,
    GParamSpec  *pspec)
{
    UfoBuffer *self = UFO_BUFFER(object);

    switch (property_id) {
        case PROP_FINISHED:
            g_value_set_boolean(value, self->priv->finished);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void ufo_buffer_finalize(GObject *gobject)
{
    UfoBuffer *self = UFO_BUFFER(gobject);
    UfoBufferPrivate *priv = UFO_BUFFER_GET_PRIVATE(self);
    if (priv->cpu_data)
        g_free(priv->cpu_data);

    G_OBJECT_CLASS(ufo_buffer_parent_class)->finalize(gobject);
}

/*
 * Type/Class Initialization
 */
static void ufo_buffer_class_init(UfoBufferClass *klass)
{
    /* override methods */
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = ufo_filter_set_property;
    gobject_class->get_property = ufo_filter_get_property;
    gobject_class->finalize = ufo_buffer_finalize;

    /* install properties */
    GParamSpec *pspec;
    pspec = g_param_spec_boolean("finished",
            "Is last buffer with invalid data and no one follows",
            "Get finished prop",
            FALSE,
            G_PARAM_READWRITE);

    g_object_class_install_property(gobject_class,
            PROP_FINISHED,
            pspec);

    /* install private data */
    g_type_class_add_private(klass, sizeof(UfoBufferPrivate));
}

static void ufo_buffer_init(UfoBuffer *self)
{
    UfoBufferPrivate *priv;
    self->priv = priv = UFO_BUFFER_GET_PRIVATE(self);
    priv->width = -1;
    priv->height = -1;
    priv->cpu_data = NULL;
    priv->state = NO_DATA;
    priv->finished = FALSE;
}
