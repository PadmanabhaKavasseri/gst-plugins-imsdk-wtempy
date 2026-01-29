#ifndef __GST_QTIGHOST_H__
#define __GST_QTIGHOST_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <Python.h>

G_BEGIN_DECLS

#define GST_TYPE_QTIGHOST (gst_qtighost_get_type())
#define GST_QTIGHOST(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QTIGHOST,GstQtiGhost))
#define GST_QTIGHOST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QTIGHOST,GstQtiGhostClass))
#define GST_IS_QTIGHOST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QTIGHOST))
#define GST_IS_QTIGHOST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QTIGHOST))

typedef struct _GstQtiGhost GstQtiGhost;
typedef struct _GstQtiGhostClass GstQtiGhostClass;

struct _GstQtiGhost {
    GstBaseTransform element;
    
    // Plugin-specific data members
    guint frame_count;
    GstVideoInfo video_info;
    
    // Python callback system
    PyObject* python_module;
    PyObject* python_function;
    gboolean python_initialized;
    
    // Properties
    gchar* python_script_path;
    gchar* python_function_name;
};

struct _GstQtiGhostClass {
    GstBaseTransformClass parent_class;
};

GType gst_qtighost_get_type(void);

G_END_DECLS

#endif /* __GST_QTIGHOST_H__ */