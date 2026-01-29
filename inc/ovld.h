#pragma once
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <gst/ml/ml-info.h>
#include <gst/video/video-converter-engine.h>


#include "ppfuncs.h"
#include "pyrelated.h"


void gst_tempy_finalize(GObject * object);

void gst_tempy_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

void gst_tempy_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

gboolean gst_tempy_set_caps(GstBaseTransform *trans, GstCaps *incaps, GstCaps *outcaps);

gboolean gst_tempy_start(GstBaseTransform * trans);

gboolean gst_tempy_decide_allocation (GstBaseTransform * base, GstQuery * query);

GstCaps * gst_tempy_transform_caps (GstBaseTransform * base, GstPadDirection direction, GstCaps * caps, GstCaps * filter);

GstFlowReturn gst_tempy_prepare_output_buffer (GstBaseTransform * base, GstBuffer * inbuffer, GstBuffer ** outbuffer);

GstFlowReturn gst_tempy_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf);

gboolean gst_tempy_stop(GstBaseTransform * trans);