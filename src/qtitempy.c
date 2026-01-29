/*******************************************************************************
-------------------------------------------------------------------------------
   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear
-------------------------------------------------------------------------------
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video.h>
#include <gst/ml/gstmlpool.h>
#include <gst/ml/gstmlmeta.h>

#include <gst/ml/ml-frame.h>
#include <gst/ml/ml-module-utils.h>
#include <gst/utils/common-utils.h>
#include <gst/utils/batch-utils.h>


#include "../inc/qtitempy.h"


#ifndef NPY_NO_DEPRECATED_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#endif
#include <numpy/arrayobject.h>

GST_DEBUG_CATEGORY(gst_tempy_debug);
#define GST_CAT_DEFAULT gst_tempy_debug

#define DEFAULT_PROP_MIN_BUFFERS     2
#define DEFAULT_PROP_MAX_BUFFERS     24

/* Type registration */
#define gst_tempy_parent_class parent_class
G_DEFINE_TYPE(GstTemPy, gst_tempy, GST_TYPE_BASE_TRANSFORM)


// Default values
#define DEFAULT_PYTHON_SCRIPT ""
#define DEFAULT_PYTHON_FUNCTION "process_frame"



// Pad capabilities - ANY
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/*******************************************************************************
-------------------------------------------------------------------------------
  Function Declrations
-------------------------------------------------------------------------------
*******************************************************************************/
static void gst_tempy_class_init(GstTemPyClass *klass);
static void gst_tempy_init(GstTemPy *self);
static gboolean plugin_init(GstPlugin *plugin);


/*******************************************************************************
-------------------------------------------------------------------------------
  GST Functions
-------------------------------------------------------------------------------
*******************************************************************************/
//Destroy and clean everything
void gst_tempy_finalize (GObject * object)
{
  GstTemPy *tempy = GST_TEMPY(object);
  
  gst_tempy_cleanup_python(tempy);
  g_free(tempy->python_script_path);
  g_free(tempy->python_function_name);
  
  G_OBJECT_CLASS(gst_tempy_parent_class)->finalize(object);
}

/* Class initialization */
static void gst_tempy_class_init(GstTemPyClass *klass) 
{

  GST_DEBUG_CATEGORY_INIT(gst_tempy_debug, "qtitempy", 0, "Template Python Debug");

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *base_class = GST_BASE_TRANSFORM_CLASS (klass);

  gst_element_class_set_details_simple (
    GST_ELEMENT_CLASS(klass),
    "QTI Sensor Post-Processor",
    "Filter",
    "Performs post-processing on gesture tensors from sensor",
    "Qualcomm Technologies, Inc."
  );

  // Set finalize, property functions
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_tempy_finalize);
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_tempy_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_tempy_get_property);

  // Install properties
  g_object_class_install_property (gobject_class, PROP_PYTHON_SCRIPT,
      g_param_spec_string("script", "Python Script", 
          "Path to Python script containing processing function",
          DEFAULT_PYTHON_SCRIPT, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property(gobject_class, PROP_PYTHON_FUNCTION,
      g_param_spec_string("function", "Python Function", 
          "Name of Python function to call for processing",
          DEFAULT_PYTHON_FUNCTION, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  // Add pad templates
  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_factory);

  base_class->start = GST_DEBUG_FUNCPTR (gst_tempy_start);
  base_class->transform_caps = GST_DEBUG_FUNCPTR (gst_tempy_transform_caps);
  base_class->set_caps = GST_DEBUG_FUNCPTR (gst_tempy_set_caps);
  base_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_tempy_decide_allocation);
  
  base_class->prepare_output_buffer = GST_DEBUG_FUNCPTR (gst_tempy_prepare_output_buffer);
  
  base_class->stop = GST_DEBUG_FUNCPTR (gst_tempy_stop);
  
  
  base_class->transform = GST_DEBUG_FUNCPTR (gst_tempy_transform);
  
}

/* Instance initialization */
static void gst_tempy_init(GstTemPy *self) {
  GST_ERROR("=== QTITEMPY INIT CALLED ===");

  gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), FALSE);

  self->srcpad = NULL;
  self->sinkpad = NULL;
  self->python_module = NULL;
  self->python_function = NULL;
  self->python_initialized = FALSE;
  self->python_script_path = g_strdup (DEFAULT_PYTHON_SCRIPT);
  self->python_function_name = g_strdup (DEFAULT_PYTHON_FUNCTION);
  self->pixlayout = DEFAULT_PROP_SUBPIXEL_LAYOUT;


  // GST_DEBUG_CATEGORY_INIT(gst_tempy_debug, "qtitempy", 0, "Template Python Debug");
}

/* Plugin registration */
static gboolean plugin_init(GstPlugin *plugin) {
  return gst_element_register(plugin, "qtitempy", GST_RANK_NONE, GST_TYPE_TEMPY);
}

GST_PLUGIN_DEFINE (
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  qtitempy,
  "Template Python Plugin",
  plugin_init,
  PACKAGE_VERSION,
  PACKAGE_LICENSE,
  PACKAGE_SUMMARY,
  PACKAGE_ORIGIN
)

