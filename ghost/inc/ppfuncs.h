#pragma once

#include <stdarg.h>

#include <gst/gst.h>

#include <gst/allocators/gstqtiallocator.h>
#include <gst/video/video-utils.h>
#include <gst/video/gstimagepool.h>
#include <gst/ml/gstmlpool.h>
#include <gst/ml/gstmlmeta.h>
#include <gst/ml/ml-module-utils.h>
#include <gst/utils/common-utils.h>
#include <gst/utils/batch-utils.h>

#ifdef HAVE_LINUX_DMA_BUF_H
#include <sys/ioctl.h>
#include <linux/dma-buf.h>
#endif // HAVE_LINUX_DMA_BUF_H

#include "pptypes.h"



typedef struct _GstTemPy GstTemPy;

GstCaps *gst_tempy_translate_ml_caps (GstTemPy * tempy, GstCaps * caps);

GstBufferPool * gst_tempy_create_pool (GstTemPy * filter, GstCaps * caps);

GstTensorLayout gst_ml_info_get_layout (GstMLInfo *mlinfo);

void init_formats(GValue * formats, ...);

void log_format_info(GstTemPy *tempy, const gchar *label, UniversalFormatInfo *format_info);

gboolean parse_format_info(GstCaps *caps, UniversalFormatInfo *format_info);

