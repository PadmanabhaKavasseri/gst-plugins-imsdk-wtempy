
#pragma once

#include <gst/gst.h>
#include "ppfuncs.h"
#include "pptypes.h"
#include <Python.h>

#ifndef NPY_NO_DEPRECATED_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#endif
#include <numpy/arrayobject.h>

typedef struct _GstTemPy GstTemPy;

extern gboolean numpy_initialized;

gboolean gst_tempy_init_python(GstTemPy * filter);
PyObject* gst_tempy_buffer_to_numpy( GstTemPy* filter, GstBuffer * buf);
gboolean gst_tempy_numpy_to_buffer(GstTemPy * filter, PyObject * result, GstBuffer * buf);
gboolean gst_tempy_numpy_to_tensor_buffer(GstTemPy *filter, PyObject *result, GstBuffer *buf);
void gst_tempy_cleanup_python(GstTemPy * filter);
PyObject* handle_tensor_buffer(GstTemPy* filter, GstMapInfo* map);
PyObject* handle_video_buffer(GstTemPy* filter, GstMapInfo* map);
PyObject* handle_raw_buffer(GstTemPy* filter, GstMapInfo* map);
void add_format_info_to_dict(PyObject* dict, const gchar* prefix, UniversalFormatInfo* format_info);
gboolean gst_tempy_numpy_to_output_buffer(GstTemPy *filter, PyObject *result, GstBuffer *buf);
gboolean gst_tempy_process_numpy_array(GstTemPy *filter, PyObject *result, GstBuffer *buf);
gboolean gst_tempy_process_dict(GstTemPy *filter, PyObject *dict, GstBuffer *buf);