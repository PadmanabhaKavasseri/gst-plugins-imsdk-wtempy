#include "ppfuncs.h"
#include "qtitempy.h"

 GstCaps *
gst_tempy_translate_ml_caps (GstTemPy * tempy, GstCaps * caps)
{
  GstCaps *result = NULL, *tmplcaps = NULL;
  GstMLInfo mlinfo;
  GstTensorLayout tensorlayout;
  gint idx = 0, length = 0;

  tmplcaps = gst_caps_new_empty ();

  if (gst_gbm_qcom_backend_is_supported ()) {
    gst_caps_append_structure_full (tmplcaps,
        gst_structure_new_empty ("video/x-raw"),
        gst_caps_features_new (GST_CAPS_FEATURE_MEMORY_GBM, NULL));
  }

  gst_caps_append_structure (tmplcaps,
      gst_structure_new_empty ("video/x-raw"));

  if (gst_caps_is_empty (caps) || gst_caps_is_any (caps))
    return tmplcaps;

  if (!gst_caps_is_fixed (caps) || !gst_ml_info_from_caps (&mlinfo, caps))
    return tmplcaps;

  result = gst_caps_new_empty ();
  length = gst_caps_get_size (tmplcaps);

  for (idx = 0; idx < length; idx++) {
    GstStructure *structure = gst_caps_get_structure (tmplcaps, idx);
    GstCapsFeatures *features = gst_caps_get_features (tmplcaps, idx);

    GValue formats = G_VALUE_INIT;
    const GValue *value = NULL;

    // If this is already expressed by the existing caps skip this structure.
    if (idx > 0 && gst_caps_is_subset_structure_full (result, structure, features))
      continue;

    // Make a copy that will be modified.
    structure = gst_structure_copy (structure);

    // Get tensor layout based on tensor dimensions
    tensorlayout = gst_ml_info_get_layout (&mlinfo);

    gst_structure_set (structure,
        "height", G_TYPE_INT, GST_ML_INFO_TENSOR_DIM_H (tensorlayout, &mlinfo),
        "width", G_TYPE_INT, GST_ML_INFO_TENSOR_DIM_W (tensorlayout, &mlinfo),
        NULL);

    // 4th dimension corresponds to the bit depth.
    if (GST_ML_INFO_TENSOR_DIM_C (tensorlayout, &mlinfo) == 1) {
      init_formats (&formats, "GRAY8", NULL);
    } else if (GST_ML_INFO_TENSOR_DIM_C (tensorlayout, &mlinfo) == 3) {
      if (tempy->pixlayout == GST_ML_VIDEO_PIXEL_LAYOUT_REGULAR) {
        if (tensorlayout.c == GST_ML_TENSOR_LAYOUT_NCHW.c)
          init_formats (&formats, "RGBP", NULL);
        else
          init_formats (&formats, "RGB", NULL);
      } else if (tempy->pixlayout == GST_ML_VIDEO_PIXEL_LAYOUT_REVERSE) {
        if (tensorlayout.c == GST_ML_TENSOR_LAYOUT_NCHW.c)
          init_formats (&formats, "BGRP", NULL);
        else
          init_formats (&formats, "BGR", NULL);
      }
    } else if (GST_ML_INFO_TENSOR_DIM_C (tensorlayout, &mlinfo) == 4) {
      if (tempy->pixlayout == GST_ML_VIDEO_PIXEL_LAYOUT_REGULAR)
        init_formats (&formats, "RGBA", "RGBx", "ARGB", "xRGB", NULL);
      else if (tempy->pixlayout == GST_ML_VIDEO_PIXEL_LAYOUT_REVERSE)
        init_formats (&formats, "BGRA", "BGRx", "ABGR", "xBGR", NULL);
    }

    gst_structure_set_value (structure, "format", &formats);
    g_value_unset (&formats);

    // Extract the frame rate from ML and propagate it to the video caps.
    value = gst_structure_get_value (gst_caps_get_structure (caps, 0), "rate");

    if (value != NULL)
      gst_structure_set_value (structure, "framerate", value);

    gst_caps_append_structure_full (result, structure,
        gst_caps_features_copy (features));
  }

  gst_caps_unref (tmplcaps);

  GST_DEBUG_OBJECT (tempy, "Returning caps: %" GST_PTR_FORMAT, result);
  return result;

}


 GstBufferPool *
gst_tempy_create_pool (GstTemPy *tempy, GstCaps * caps)
{

  GstStructure *structure = gst_caps_get_structure(caps, 0);
  const gchar *media_type = gst_structure_get_name(structure);
  
  

  GstBufferPool *pool = NULL;
  GstStructure *config = NULL;
  GstAllocator *allocator = NULL;
  GstMLInfo info;

  if (g_str_has_prefix(media_type, "text")) {
    GST_DEBUG_OBJECT (tempy, "Creating simple buffer pool for text caps");
    
    GstBufferPool *pool = gst_buffer_pool_new();
    GstStructure *config = gst_buffer_pool_get_config(pool);
    
    // Text buffer size - reasonable default (4KB should be plenty for text)
    gsize buffer_size = 4096;
    
    gst_buffer_pool_config_set_params(config, caps, buffer_size,
        DEFAULT_PROP_MIN_BUFFERS, DEFAULT_PROP_MAX_BUFFERS);
    
    if (!gst_buffer_pool_set_config(pool, config)) {
      GST_WARNING_OBJECT(tempy, "Failed to set text buffer pool configuration!");
      g_clear_object(&pool);
      return NULL;
    }
    
    return pool;  // Return actual pool, not NULL!
  }

  if (!gst_ml_info_from_caps (&info, caps)) {
    GST_ERROR ("Invalid caps %" GST_PTR_FORMAT, caps);
    return NULL;
  }

  GST_DEBUG_OBJECT (tempy, "Create buffer pool based on caps: %"
      GST_PTR_FORMAT, caps);

  GST_INFO ("Uses DMA memory");
  pool = gst_ml_buffer_pool_new (GST_ML_BUFFER_POOL_TYPE_DMA);

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, caps, gst_ml_info_size (&info),
      DEFAULT_PROP_MIN_BUFFERS, DEFAULT_PROP_MAX_BUFFERS);

  allocator = gst_fd_allocator_new ();

  gst_buffer_pool_config_set_allocator (config, allocator, NULL);
  gst_buffer_pool_config_add_option (
      config, GST_ML_BUFFER_POOL_OPTION_TENSOR_META);

  if (!gst_buffer_pool_set_config (pool, config)) {
    GST_WARNING ("Failed to set pool configuration!");
    g_clear_object (&pool);
  }
  g_object_unref (allocator);

  return pool;
}

GstTensorLayout
gst_ml_info_get_layout (GstMLInfo *mlinfo)
{
  if (GST_ML_INFO_N_DIMENSIONS (mlinfo, 0) == 5) {
    return GST_ML_TENSOR_LAYOUT_NDHWC;
  } else if ((GST_ML_INFO_TENSOR_DIM (mlinfo, 0, 3) > 4) &&
      (GST_ML_INFO_TENSOR_DIM (mlinfo, 0, 1) <= 4)) {
    return GST_ML_TENSOR_LAYOUT_NCHW;
  }
  return GST_ML_TENSOR_LAYOUT_NHWC;
}

void
init_formats (GValue * formats, ...)
{
  GValue format = G_VALUE_INIT;
  gchar *string = NULL;
  va_list args;

  g_value_init (formats, GST_TYPE_LIST);
  va_start (args, formats);

  while ((string = va_arg (args, gchar *))) {
    g_value_init (&format, G_TYPE_STRING);
    g_value_set_string (&format, string);

    gst_value_list_append_value (formats, &format);
    g_value_unset (&format);
  }

  va_end (args);
}

// Helper function to parse any format using GStreamer APIs
gboolean parse_format_info(GstCaps *caps, UniversalFormatInfo *format_info) 
{
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *media_type = gst_structure_get_name(structure);
    
    if (g_str_has_prefix(media_type, "video")) {
        // Parse video format using GStreamer Video API
        format_info->type = FORMAT_TYPE_VIDEO;
        return gst_video_info_from_caps(&format_info->info.video, caps);
        
    } else if (g_str_has_prefix(media_type, "neural-network")) {
        // Parse tensor format using GStreamer ML API
        format_info->type = FORMAT_TYPE_TENSOR;
        gst_ml_info_init(&format_info->info.tensor);  // Initialize first
        return gst_ml_info_from_caps(&format_info->info.tensor, caps);  // ← This is the function!
        
    } else if (g_str_has_prefix(media_type, "text")) {
        // NEW: Handle text format
        format_info->type = FORMAT_TYPE_TEXT;        
        GST_DEBUG("Handling text/x-raw as UTF-8 bytes");
        return TRUE;
        
    } else {
        // Unknown format
        format_info->type = FORMAT_TYPE_UNKNOWN;
        GST_WARNING("Unknown media type: %s", media_type);
        return FALSE;
    }
}

// Helper function to log format info
void log_format_info(GstTemPy *tempy, const gchar *label, UniversalFormatInfo *format_info)
{
    switch (format_info->type) {
        case FORMAT_TYPE_VIDEO:
          GST_INFO_OBJECT(tempy, "%s: Video %dx%d, format: %s", 
              label,
              GST_VIDEO_INFO_WIDTH(&format_info->info.video),
              GST_VIDEO_INFO_HEIGHT(&format_info->info.video),
              gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&format_info->info.video)));
          break;
            
        case FORMAT_TYPE_TENSOR:
          GST_INFO_OBJECT(tempy, "%s: Tensor type: %d, n_tensors: %u", 
              label,
              GST_ML_INFO_TYPE(&format_info->info.tensor),
              GST_ML_INFO_N_TENSORS(&format_info->info.tensor));
          
          // Log tensor dimensions
          for (guint i = 0; i < GST_ML_INFO_N_TENSORS(&format_info->info.tensor); i++) {
              GST_INFO_OBJECT(tempy, "  Tensor[%u]: %u dimensions", 
                  i, GST_ML_INFO_N_DIMENSIONS(&format_info->info.tensor, i));
              
              for (guint j = 0; j < GST_ML_INFO_N_DIMENSIONS(&format_info->info.tensor, i); j++) {
                  GST_INFO_OBJECT(tempy, "    Dim[%u]: %u", 
                      j, GST_ML_INFO_TENSOR_DIM(&format_info->info.tensor, i, j));
              }
          }
          break;

        case FORMAT_TYPE_TEXT:
          GST_INFO_OBJECT(tempy, "%s: Text (UTF-8)", label);
          break;
        default:
            GST_INFO_OBJECT(tempy, "%s: Unknown format", label);
            break;
    }
}

// Helper function to add format info to Python dictionary
void add_format_info_to_dict(PyObject* dict, const gchar* prefix, UniversalFormatInfo* format_info)
{
    gchar* key;
    
    switch (format_info->type) {
        case FORMAT_TYPE_VIDEO:
            // Add video format information
            key = g_strdup_printf("%s_type", prefix);
            PyDict_SetItemString(dict, key, PyUnicode_FromString("video"));
            g_free(key);
            
            key = g_strdup_printf("%s_width", prefix);
            PyDict_SetItemString(dict, key, PyLong_FromLong(GST_VIDEO_INFO_WIDTH(&format_info->info.video)));
            g_free(key);
            
            key = g_strdup_printf("%s_height", prefix);
            PyDict_SetItemString(dict, key, PyLong_FromLong(GST_VIDEO_INFO_HEIGHT(&format_info->info.video)));
            g_free(key);
            
            key = g_strdup_printf("%s_format", prefix);
            PyDict_SetItemString(dict, key, PyUnicode_FromString(
                gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&format_info->info.video))));
            g_free(key);
            break;
            
        case FORMAT_TYPE_TENSOR:
            // Add tensor format information
            key = g_strdup_printf("%s_type", prefix);
            PyDict_SetItemString(dict, key, PyUnicode_FromString("tensor"));
            g_free(key);
            
            key = g_strdup_printf("%s_ml_type", prefix);
            PyDict_SetItemString(dict, key, PyLong_FromLong(GST_ML_INFO_TYPE(&format_info->info.tensor)));
            g_free(key);
            
            key = g_strdup_printf("%s_n_tensors", prefix);
            PyDict_SetItemString(dict, key, PyLong_FromLong(GST_ML_INFO_N_TENSORS(&format_info->info.tensor)));
            g_free(key);
            
            // Add tensor dimensions as a list
            PyObject* dimensions_list = PyList_New(0);
            for (guint i = 0; i < GST_ML_INFO_N_TENSORS(&format_info->info.tensor); i++) {
                PyObject* tensor_dims = PyList_New(0);
                for (guint j = 0; j < GST_ML_INFO_N_DIMENSIONS(&format_info->info.tensor, i); j++) {
                    PyList_Append(tensor_dims, PyLong_FromLong(GST_ML_INFO_TENSOR_DIM(&format_info->info.tensor, i, j)));
                }
                PyList_Append(dimensions_list, tensor_dims);
            }
            key = g_strdup_printf("%s_dimensions", prefix);
            PyDict_SetItemString(dict, key, dimensions_list);
            g_free(key);
            break;
            
        default:
            key = g_strdup_printf("%s_type", prefix);
            PyDict_SetItemString(dict, key, PyUnicode_FromString("unknown"));
            g_free(key);
            break;
    }
}