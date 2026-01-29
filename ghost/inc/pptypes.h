#pragma once

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <gst/ml/ml-info.h>
#include <gst/video/video-converter-engine.h>


#define DEFAULT_PROP_MIN_BUFFERS     2
#define DEFAULT_PROP_MAX_BUFFERS     24
#define DEFAULT_PROP_SAMPLE_RATE     16000
#define DEFAULT_PROP_CONVERSION_FEATURE   GST_AUDIO_FEATURE_RAW
#define DEFAULT_PROP_PARAMS          NULL

#define DEFAULT_PROP_SUBPIXEL_LAYOUT   GST_ML_VIDEO_PIXEL_LAYOUT_REGULAR

#define GST_ML_VIDEO_FORMATS \
    "{ RGBA, BGRA, ABGR, ARGB, RGBx, BGRx, xRGB, xBGR, BGR, RGB, GRAY8, NV12, NV21, YUY2, UYVY, NV12_Q08C }"

#define GST_ML_TENSOR_TYPES "{ INT8, UINT8, INT16, UINT16, INT32, UINT32, FLOAT16, FLOAT32 }"

#define GST_ML_VIDEO_CONVERTER_SRC_CAPS    \
    "neural-network/tensors, "             \
    "type = (string) " GST_ML_TENSOR_TYPES

#define GST_ML_TENSOR_LAYOUT_NHWC \
    (GstTensorLayout){ .n = 0, .d = -1, .h = 1, .w = 2, .c = 3 }
#define GST_ML_TENSOR_LAYOUT_NCHW \
    (GstTensorLayout){ .n = 0, .d = -1, .h = 2, .w = 3, .c = 1 }
#define GST_ML_TENSOR_LAYOUT_NDHWC \
    (GstTensorLayout){ .n = 0, .d = 1, .h = 2, .w = 3, .c = 4 }

#define GST_ML_INFO_TENSOR_DIM_N(tensorlayout, mlinfo) \
    GST_ML_INFO_TENSOR_DIM(mlinfo, 0, tensorlayout.n)
#define GST_ML_INFO_TENSOR_DIM_D(tensorlayout, mlinfo) \
    ((tensorlayout.d == -1) ? 1 : \
        GST_ML_INFO_TENSOR_DIM(mlinfo, 0, tensorlayout.d))
#define GST_ML_INFO_TENSOR_DIM_H(tensorlayout, mlinfo) \
    GST_ML_INFO_TENSOR_DIM(mlinfo, 0, tensorlayout.h)
#define GST_ML_INFO_TENSOR_DIM_W(tensorlayout, mlinfo) \
    GST_ML_INFO_TENSOR_DIM(mlinfo, 0, tensorlayout.w)
#define GST_ML_INFO_TENSOR_DIM_C(tensorlayout, mlinfo) \
    GST_ML_INFO_TENSOR_DIM(mlinfo, 0, tensorlayout.c)


typedef enum {
  GST_ML_VIDEO_PIXEL_LAYOUT_REGULAR,
  GST_ML_VIDEO_PIXEL_LAYOUT_REVERSE,
} GstVideoPixelLayout;

typedef struct {
  gint n;
  gint d;
  gint h;
  gint w;
  gint c;
} GstTensorLayout;


typedef enum {
    FORMAT_TYPE_UNKNOWN,
    FORMAT_TYPE_VIDEO,
    FORMAT_TYPE_TENSOR, 
    FORMAT_TYPE_TEXT
} FormatType;

typedef union {
    GstVideoInfo video;
    GstMLInfo tensor;
} FormatInfo;

typedef struct {
    FormatType type;
    FormatInfo info;
} UniversalFormatInfo;