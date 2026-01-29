
#include "ovld.h"
#include "qtitempy.h"

#define GST_CAT_DEFAULT gst_tempy_debug

void 
gst_tempy_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstTemPy *filter = GST_TEMPY(object);
  
  switch (prop_id) {
      case PROP_PYTHON_SCRIPT:
          g_free(filter->python_script_path);
          filter->python_script_path = g_value_dup_string(value);
          break;
      case PROP_PYTHON_FUNCTION:
          g_free(filter->python_function_name);
          filter->python_function_name = g_value_dup_string(value);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
          break;
  }
}

void 
gst_tempy_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstTemPy *filter = GST_TEMPY(object);
  
  switch (prop_id) {
      case PROP_PYTHON_SCRIPT:
          g_value_set_string(value, filter->python_script_path);
          break;
      case PROP_PYTHON_FUNCTION:
          g_value_set_string(value, filter->python_function_name);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
          break;
  }
}

gboolean 
gst_tempy_start (GstBaseTransform * base)
{
  GST_ERROR("=== QTITEMPY START CALLED ===");
  GstTemPy *tempy = GST_TEMPY(base);
  return gst_tempy_init_python(tempy);
}

GstCaps* 
gst_tempy_transform_caps (GstBaseTransform *base, GstPadDirection direction,
                          GstCaps *caps, GstCaps *filter) 
{

  // GST_ERROR("=== TRANSFORM_CAPS CALLED - Direction: %s ===", 
  //           (direction == GST_PAD_SINK) ? "SINK" : "SRC");
  // "I can convert between any formats - Python will handle it"
  GstCaps *result;
  
  if (direction == GST_PAD_SINK) {
    // "Given any input, I can produce what you want"
    if (filter) {
      result = gst_caps_copy(filter);  // "I'll produce exactly what you want"
    } else {
      result = gst_caps_new_any();     // "I can produce anything"
    }
  } else {
    // "To produce any output, I'll accept anything"
    result = gst_caps_new_any();         // "I accept any input"
  }
  
  return result;
}


gboolean 
gst_tempy_set_caps (GstBaseTransform *base, GstCaps *incaps, GstCaps *outcaps) 
{
    GstTemPy *tempy = GST_TEMPY (base);
    
    GST_INFO_OBJECT (tempy, "Setting caps - Input: %" GST_PTR_FORMAT, incaps);
    GST_INFO_OBJECT (tempy, "Setting caps - Output: %" GST_PTR_FORMAT, outcaps);
    
    // Parse INPUT format
    if (!parse_format_info(incaps, &tempy->input_format)) {
        GST_ERROR_OBJECT (tempy, "Failed to parse input caps");
        return FALSE;
    }
    
    // Parse OUTPUT format  
    if (!parse_format_info(outcaps, &tempy->output_format)) {
        GST_ERROR_OBJECT (tempy, "Failed to parse output caps");
        return FALSE;
    }
    
    // Log what we parsed
    log_format_info(tempy, "Input", &tempy->input_format);
    log_format_info(tempy, "Output", &tempy->output_format);
    
    return TRUE;
}

gboolean
gst_tempy_decide_allocation (GstBaseTransform * base, GstQuery * query)
{
  GstTemPy *tempy = GST_TEMPY (base);

  GstCaps *caps = NULL;
  GstBufferPool *pool = NULL;
  GstStructure *config = NULL;
  GstAllocator *allocator = NULL;
  guint size, minbuffers, maxbuffers;
  GstAllocationParams params;

  // These caps tell us what format/size buffers we need to allocate
  gst_query_parse_allocation (query, &caps, NULL);
  if (!caps) {
      GST_ERROR ("Failed to parse the allocation caps!");
      return FALSE;
  }

  // ADD THIS:
  GST_DEBUG_OBJECT(tempy, "=== DECIDE_ALLOCATION DEBUG ===");
  GST_DEBUG_OBJECT(tempy, "Output caps: %" GST_PTR_FORMAT, caps);

  GstStructure *structure = gst_caps_get_structure(caps, 0);
  const gchar *media_type = gst_structure_get_name(structure);
  GST_DEBUG_OBJECT(tempy, "Media type: %s", media_type);

  // Check if downstream already proposed a buffer pool
  if (gst_query_get_n_allocation_pools (query) > 0)
    gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);

  // Create a new pool in case none was proposed in the query.
  if (!pool && !(pool = gst_tempy_create_pool (tempy, caps))) {
    GST_DEBUG ("No custom buffer pool created, using default");  // Change ERROR to DEBUG
    return TRUE;
  }
  // ADD THIS:
  GST_DEBUG_OBJECT(tempy, "Created pool: %p", pool);  

  // Invalidate the cached pool if there is an allocation_query.
  if (tempy->outpool) {
    gst_object_unref (tempy->outpool);
  }

  tempy->outpool = pool;

  // Get the configured pool properties in order to set in query.
  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_get_params (config, &caps, &size, &minbuffers,
      &maxbuffers);

  GST_DEBUG_OBJECT(tempy, "Pool config - size: %u, min: %u, max: %u", size, minbuffers, maxbuffers);
  GST_DEBUG_OBJECT(tempy, "Pool caps: %" GST_PTR_FORMAT, caps);

  if (gst_buffer_pool_config_get_allocator (config, &allocator, &params))
    gst_query_add_allocation_param (query, allocator, &params);

  gst_structure_free (config);

  // Check whether the query has pool.
  if (gst_query_get_n_allocation_pools (query) > 0)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, minbuffers,
        maxbuffers);
  else
    gst_query_add_allocation_pool (query, pool, size, minbuffers, maxbuffers);

  gst_query_add_allocation_meta (query, GST_ML_TENSOR_META_API_TYPE, NULL);

  return TRUE;
}



GstFlowReturn
gst_tempy_prepare_output_buffer (GstBaseTransform * base, 
    GstBuffer * inbuffer, GstBuffer ** outbuffer)
{
  GstTemPy *tempy = GST_TEMPY (base);
  GstBufferPool *pool = tempy->outpool;

  if (gst_base_transform_is_passthrough (base)) {
    GST_TRACE ("Passthrough, no need to do anything");
    *outbuffer = inbuffer;
    return GST_FLOW_OK;
  }

  if (!gst_buffer_pool_is_active (pool) &&
      !gst_buffer_pool_set_active (pool, TRUE)) {
    GST_ERROR ("Failed to activate output buffer pool!");
    return GST_FLOW_ERROR;
  }

  // Input is marked as GAP, nothing to process. Create a GAP output buffer.
  if (gst_buffer_get_size (inbuffer) == 0 &&
      GST_BUFFER_FLAG_IS_SET (inbuffer, GST_BUFFER_FLAG_GAP)) {
    *outbuffer = gst_buffer_new ();
    GST_BUFFER_FLAG_SET (*outbuffer, GST_BUFFER_FLAG_GAP);
  }

  if ((*outbuffer == NULL) &&
      gst_buffer_pool_acquire_buffer (pool, outbuffer, NULL) != GST_FLOW_OK) {
    GST_ERROR ("Failed to acquire output buffer!");
    return GST_FLOW_ERROR;
  }

  // Copy the flags and timestamps from the input buffer.
  gst_buffer_copy_into (*outbuffer, inbuffer,
      (GstBufferCopyFlags)(GST_BUFFER_COPY_METADATA | GST_BUFFER_COPY_TIMESTAMPS),
      0, -1);

  // Copy the offset field as it may contain channels data for batched buffers.
  GST_BUFFER_OFFSET (*outbuffer) = GST_BUFFER_OFFSET (inbuffer);

  if (gst_buffer_get_size (*outbuffer) == 0)
      GST_BUFFER_FLAG_SET (*outbuffer, GST_BUFFER_FLAG_GAP);

  return GST_FLOW_OK;
}



GstFlowReturn 
gst_tempy_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf) 
{
  GstTemPy *filter = GST_TEMPY(trans);
  
  filter->frame_count++;
  
  // If no Python function is loaded, return error
  if (!filter->python_function) {
    GST_ERROR_OBJECT(filter, "No Python function loaded");
    return GST_FLOW_ERROR;
  }
  
  // Acquire GIL (Global Interpreter Lock) for thread safety
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  // Convert input buffer to NumPy array
  PyObject* input_array = gst_tempy_buffer_to_numpy(filter, inbuf);
  if (!input_array) {
    GST_ERROR_OBJECT(filter, "Failed to convert input buffer to NumPy array");
    PyGILState_Release(gstate);
    return GST_FLOW_ERROR;
  }
  
  // Create comprehensive format info dictionary
  PyObject* frame_info = PyDict_New();
  PyDict_SetItemString(frame_info, "data", input_array);
  
  // Add INPUT format information
  add_format_info_to_dict(frame_info, "input", &filter->input_format);
  
  // Add OUTPUT format information  
  add_format_info_to_dict(frame_info, "output", &filter->output_format);
  
  Py_DECREF(input_array); // We don't need the direct reference anymore

  GstClockTime start_time = gst_util_get_timestamp();
  
  // Call Python function with comprehensive format info
  PyObject* args = PyTuple_New(1);
  PyTuple_SetItem(args, 0, frame_info); // steals reference
  
  PyObject* result = PyObject_CallObject(filter->python_function, args);
  Py_DECREF(args);
  
  if (!result) {
    GST_ERROR_OBJECT(filter, "Python function call failed");
    PyErr_Print();
    PyGILState_Release(gstate);
    return GST_FLOW_ERROR;
  }

  GstClockTime end_time = gst_util_get_timestamp();
  gdouble duration_ms = (gdouble)(end_time - start_time) / GST_MSECOND;
  // GST_DEBUG_OBJECT(filter, "Python processing took %.3f ms", duration_ms);
  
  // Convert result to output buffer
  gboolean success = gst_tempy_numpy_to_output_buffer(filter, result, outbuf);
  Py_DECREF(result);

  // Release GIL
  PyGILState_Release(gstate);
  
  if (!success) {
    GST_ERROR_OBJECT(filter, "Failed to convert Python result to output buffer");
    return GST_FLOW_ERROR;
  }
  
  // GST_DEBUG_OBJECT(filter, "Frame #%u processed successfully", filter->frame_count);
  return GST_FLOW_OK;
}

gboolean 
gst_tempy_stop(GstBaseTransform * trans)
{
  GstTemPy *filter = GST_TEMPY(trans);
  gst_tempy_cleanup_python(filter);
  return TRUE;
}
