/*******************************************************************************
-------------------------------------------------------------------------------
   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear
-------------------------------------------------------------------------------
*******************************************************************************/

#ifndef __GST_QTI_TEMPY_H__
#define __GST_QTI_TEMPY_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <Python.h>
#include <gst/ml/ml-frame.h>

#include "ovld.h"
#include "ppfuncs.h"
#include "pptypes.h"
#include "pyrelated.h"

G_BEGIN_DECLS

#define GST_TYPE_TEMPY (gst_tempy_get_type())
G_DECLARE_FINAL_TYPE(GstTemPy, gst_tempy, GST, TEMPY, GstBaseTransform);

GST_DEBUG_CATEGORY_EXTERN(gst_tempy_debug);

// Properties
enum {
    PROP_0,
    PROP_PYTHON_SCRIPT,
    PROP_PYTHON_FUNCTION
};  

struct _GstTemPy {
  GstBaseTransform parent;
  GstPad *srcpad;
  GstPad *sinkpad;
  GstBufferPool *outpool;
  // GstMLInfo *ml_info;

  guint stage_id;

  // Plugin-specific data members
  guint frame_count;
  // GstVideoInfo video_info;
  // Format information
  UniversalFormatInfo input_format;
  UniversalFormatInfo output_format;

  // Python callback system
  PyObject* python_module;
  PyObject* python_function;
  gboolean python_initialized;
  
  // Properties
  gchar* python_script_path;
  gchar* python_function_name;

  GstVideoPixelLayout  pixlayout;
  
  
};

struct _GstTemPyClass {
  GstBaseTransformClass parent_class;
};

G_END_DECLS

#endif // __GST_TEMPY_H__
