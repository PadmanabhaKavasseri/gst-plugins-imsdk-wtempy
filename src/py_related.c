#include "pyrelated.h"
#include "qtitempy.h" 

// Global flag for NumPy initialization
gboolean numpy_initialized = FALSE;

gboolean gst_tempy_init_python(GstTemPy * filter)
{
	if (filter->python_initialized) {
		return TRUE;
	}
	
	// Ensure Python symbols are global (if using dlopen solution)
	// ensure_python_symbols_global();
	
	// Initialize Python
	if (!Py_IsInitialized()) {
		Py_Initialize();
		
		// IMPORTANT: Initialize threading support
		if (!PyEval_ThreadsInitialized()) {
			PyEval_InitThreads();
		}
		
		if (PyErr_Occurred()) {
			GST_ERROR_OBJECT(filter, "Failed to initialize Python");
			PyErr_Print();
			return FALSE;
		}
	}
	
	// Ensure we can use threading
	PyEval_SaveThread();  // Release the GIL
	
	// Re-acquire for our initialization
	PyGILState_STATE gstate = PyGILState_Ensure();
	
	// Initialize NumPy
	if (!numpy_initialized) {
		import_array1(FALSE);
		numpy_initialized = TRUE;
		GST_INFO_OBJECT(filter, "NumPy C API initialized");
	}
	
	// Test NumPy import
	PyObject* numpy = PyImport_ImportModule("numpy");
	if (!numpy) {
		GST_ERROR_OBJECT(filter, "Failed to import numpy module");
		PyErr_Print();
		PyGILState_Release(gstate);
		return FALSE;
	}
	Py_DECREF(numpy);
	
	// Load the Python script if specified
	if (filter->python_script_path && strlen(filter->python_script_path) > 0) {
		// ... (rest of your script loading code)
		
		// Add script directory to Python path
		PyObject* sys = PyImport_ImportModule("sys");
		if (!sys) {
			GST_ERROR_OBJECT(filter, "Failed to import sys module");
			PyErr_Print();
			PyGILState_Release(gstate);
			return FALSE;
		}
		
		PyObject* path = PyObject_GetAttrString(sys, "path");
		if (!path) {
			Py_DECREF(sys);
			GST_ERROR_OBJECT(filter, "Failed to get sys.path");
			PyGILState_Release(gstate);
			return FALSE;
		}
		
		gchar* script_dir = g_path_get_dirname(filter->python_script_path);
		PyObject* py_script_dir = PyUnicode_FromString(script_dir);
		PyList_Insert(path, 0, py_script_dir);
		
		Py_DECREF(py_script_dir);
		Py_DECREF(path);
		Py_DECREF(sys);
		g_free(script_dir);
		
		// Import the module
		gchar* module_name = g_path_get_basename(filter->python_script_path);
		if (g_str_has_suffix(module_name, ".py")) {
			module_name[strlen(module_name) - 3] = '\0';
		}
		
		GST_INFO_OBJECT(filter, "Loading Python module: %s", module_name);
		filter->python_module = PyImport_ImportModule(module_name);
		g_free(module_name);
		
		if (!filter->python_module) {
			PyErr_Print();
			GST_ERROR_OBJECT(filter, "Failed to load Python module");
			PyGILState_Release(gstate);
			return FALSE;
		}
		
		// Get the processing function
		filter->python_function = PyObject_GetAttrString(filter->python_module, filter->python_function_name);
		if (!filter->python_function || !PyCallable_Check(filter->python_function)) {
			GST_ERROR_OBJECT(filter, "Python function '%s' not found or not callable", filter->python_function_name);
			PyGILState_Release(gstate);
			return FALSE;
		}
	}
	
	PyGILState_Release(gstate);
	
	filter->python_initialized = TRUE;
	GST_INFO_OBJECT(filter, "Python initialized successfully");
	return TRUE;
}

PyObject* gst_tempy_buffer_to_numpy(GstTemPy* filter, GstBuffer * buf)
{
    GstMapInfo map;
    PyObject* result = NULL;
    
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(filter, "Failed to map buffer");
        return NULL;
    }
    
    // Use the new UniversalFormatInfo to determine input format
    switch (filter->input_format.type) {
        case FORMAT_TYPE_VIDEO:
            GST_DEBUG_OBJECT(filter, "Processing video input buffer");
            result = handle_video_buffer(filter, &map);
            break;
            
        case FORMAT_TYPE_TENSOR:
            GST_DEBUG_OBJECT(filter, "Processing tensor input buffer");
            result = handle_tensor_buffer(filter, &map);
            break;
            
        default:
            GST_ERROR_OBJECT(filter, "Unknown input format type: %d", filter->input_format.type);
            result = handle_raw_buffer(filter, &map);  // Fallback to raw buffer
            break;
    }
    
    gst_buffer_unmap(buf, &map);
    return result;
}

// Updated handle_video_buffer to use UniversalFormatInfo
PyObject* handle_video_buffer(GstTemPy* filter, GstMapInfo* map)
{
    // Get video info from the universal format structure
    GstVideoInfo* video_info = &filter->input_format.info.video;
    
    // Get video dimensions
    gint width = GST_VIDEO_INFO_WIDTH(video_info);
    gint height = GST_VIDEO_INFO_HEIGHT(video_info);
    GstVideoFormat format = GST_VIDEO_INFO_FORMAT(video_info);
    
    gint channels;
    gsize expected_size;
    PyObject* array = NULL;
    
    // Declare array dimensions outside switch
    npy_intp dims_1d[1];
    npy_intp dims_3d[3];
    
    switch (format) {
        case GST_VIDEO_FORMAT_NV12:
            // NV12: Pass as 1D raw buffer, let Python handle conversion
            channels = 1;
            expected_size = width * height * 3 / 2;
            
            dims_1d[0] = map->size;
            array = PyArray_SimpleNew(1, dims_1d, NPY_UINT8);
            break;
            
        case GST_VIDEO_FORMAT_RGB:
        case GST_VIDEO_FORMAT_BGR:
            channels = 3;
            expected_size = width * height * 3;
            
            dims_3d[0] = height;
            dims_3d[1] = width;
            dims_3d[2] = channels;
            array = PyArray_SimpleNew(3, dims_3d, NPY_UINT8);
            break;
            
        case GST_VIDEO_FORMAT_RGBA:
        case GST_VIDEO_FORMAT_BGRA:
            channels = 4;
            expected_size = width * height * 4;
            
            dims_3d[0] = height;
            dims_3d[1] = width;
            dims_3d[2] = channels;
            array = PyArray_SimpleNew(3, dims_3d, NPY_UINT8);
            break;
            
        default:
            GST_ERROR_OBJECT(filter, "Unsupported video format: %s", 
                gst_video_format_to_string(format));
            return NULL;
    }
    
    if (!array) {
        GST_ERROR_OBJECT(filter, "Failed to create NumPy array");
        return NULL;
    }
    
    // Verify buffer size (allow padding)
    if (map->size < expected_size) {
        GST_ERROR_OBJECT(filter, "Buffer too small: got %zu, need at least %zu", 
            map->size, expected_size);
        Py_DECREF(array);
        return NULL;
    } else if (map->size > expected_size) {
        GST_DEBUG_OBJECT(filter, "Buffer has padding: got %zu, expected %zu", 
            map->size, expected_size);
    }
    
    // Copy data from GStreamer buffer to NumPy array
    unsigned char* array_data = (unsigned char*)PyArray_DATA((PyArrayObject*)array);
    memcpy(array_data, map->data, MIN(map->size, expected_size));
    
    GST_DEBUG_OBJECT(filter, "Created video NumPy array: %dx%dx%d", height, width, channels);
    return array;
}

// Keep handle_tensor_buffer the same (it already works)
PyObject* handle_tensor_buffer(GstTemPy* filter, GstMapInfo* map)
{
    // Create 1D numpy array from raw tensor data
    npy_intp dims[1] = { map->size };
    PyObject* array = PyArray_SimpleNew(1, dims, NPY_UINT8);
    
    if (!array) {
        GST_ERROR_OBJECT(filter, "Failed to create tensor NumPy array");
        return NULL;
    }
    
    // Copy tensor data
    unsigned char* array_data = (unsigned char*)PyArray_DATA((PyArrayObject*)array);
    memcpy(array_data, map->data, map->size);
    
    GST_DEBUG_OBJECT(filter, "Created tensor NumPy array: %zu bytes", map->size);
    return array;
}

// New fallback function for unknown formats
PyObject* handle_raw_buffer(GstTemPy* filter, GstMapInfo* map)
{
    GST_WARNING_OBJECT(filter, "Handling unknown format as raw buffer");
    
    // Create 1D numpy array from raw data
    npy_intp dims[1] = { map->size };
    PyObject* array = PyArray_SimpleNew(1, dims, NPY_UINT8);
    
    if (!array) {
        GST_ERROR_OBJECT(filter, "Failed to create raw NumPy array");
        return NULL;
    }
    
    // Copy raw data
    unsigned char* array_data = (unsigned char*)PyArray_DATA((PyArrayObject*)array);
    memcpy(array_data, map->data, map->size);
    
    GST_DEBUG_OBJECT(filter, "Created raw NumPy array: %zu bytes", map->size);
    return array;
}

gboolean gst_tempy_numpy_to_output_buffer(GstTemPy *filter, PyObject *result, GstBuffer *buf) {
    
    if (PyDict_Check(result)) {
		// text output
        return gst_tempy_process_dict(filter, result, buf);
    }
    else if (PyArray_Check(result)) {
        // video or tensor output
        return gst_tempy_process_numpy_array(filter, result, buf);
    }
    else {
        GST_ERROR_OBJECT(filter, "Unsupported Python return type");
        return FALSE;
    }
}

gboolean gst_tempy_process_dict(GstTemPy *filter, PyObject *dict, GstBuffer *buf) {
    // At the start of gst_tempy_process_dict
    GST_ERROR_OBJECT(filter, "=== PROCESS_DICT CALLED ===");
    GST_DEBUG_OBJECT(filter, "Processing Python dictionary - extracting hardcoded values");
    
    // Get output format info to verify we're outputting text
    GstCaps *outcaps = gst_pad_get_current_caps(GST_BASE_TRANSFORM_SRC_PAD(GST_BASE_TRANSFORM(filter)));
    GstStructure *structure = gst_caps_get_structure(outcaps, 0);
    const gchar *media_type = gst_structure_get_name(structure);
    
    if (!g_str_has_prefix(media_type, "text")) {
        GST_ERROR_OBJECT(filter, "Dictionary return requires text/x-raw output, got: %s", media_type);
        gst_caps_unref(outcaps);
        return FALSE;
    }
    
    gst_caps_unref(outcaps);

    GST_DEBUG_OBJECT(filter, "Building Qualcomm-style serialized structure from dictionary");

    /* -----------------------------
    * Create the outer LIST
    * ----------------------------- */
    GValue list = G_VALUE_INIT;
    g_value_init(&list, GST_TYPE_LIST);

    /* -----------------------------
    * Create ObjectDetection struct
    * ----------------------------- */
    GstStructure *det = gst_structure_new_empty("ObjectDetection");

    /* -----------------------------
    * Create bounding-boxes ARRAY
    * ----------------------------- */
    GValue bbox_array = G_VALUE_INIT;
    g_value_init(&bbox_array, GST_TYPE_ARRAY);

    /* -----------------------------
    * Extract detections from Python dictionary
    * ----------------------------- */
    PyObject *detections_list = PyDict_GetItemString(dict, "detections");
    if (detections_list && PyList_Check(detections_list)) {
        
        Py_ssize_t num_detections = PyList_Size(detections_list);
        GST_ERROR_OBJECT(filter, "Processing %zd detections from dictionary", num_detections);
        
        for (Py_ssize_t i = 0; i < num_detections; i++) {
            PyObject *detection_dict = PyList_GetItem(detections_list, i);
            
            if (PyDict_Check(detection_dict)) {
                // Extract detection data from Python dictionary
                PyObject *bbox_obj = PyDict_GetItemString(detection_dict, "bbox");
                PyObject *conf_obj = PyDict_GetItemString(detection_dict, "confidence");
                PyObject *class_name_obj = PyDict_GetItemString(detection_dict, "class_name");
                PyObject *id_obj = PyDict_GetItemString(detection_dict, "id");
                
                if (bbox_obj && PyList_Check(bbox_obj) && PyList_Size(bbox_obj) == 4 &&
                    conf_obj && class_name_obj && id_obj) {
                    
                    // Extract values from Python
                    const char *class_name = PyUnicode_AsUTF8(class_name_obj);
                    double confidence = PyFloat_AsDouble(conf_obj);
                    guint id = (guint)PyLong_AsLong(id_obj);
                    
                    GST_DEBUG_OBJECT(filter, "Extracted from Python: %s, conf=%.2f, id=%u", 
                                   class_name, confidence, id);
                    
                    // Create bounding box structure with extracted values
                    GstStructure *bbox = gst_structure_new(
                        class_name,
                        "id", G_TYPE_UINT, id,
                        "confidence", G_TYPE_DOUBLE, confidence,
                        "color", G_TYPE_UINT, 16711935, // Keep hardcoded color
                        NULL
                    );

                    /* Rectangle array from Python bbox list */
                    GValue rect = G_VALUE_INIT;
                    g_value_init(&rect, GST_TYPE_ARRAY);

                    GValue v = G_VALUE_INIT;
                    g_value_init(&v, G_TYPE_FLOAT);

                    for (int j = 0; j < 4; j++) {
                        PyObject *coord = PyList_GetItem(bbox_obj, j);
                        float coord_val = (float)PyFloat_AsDouble(coord);
                        g_value_set_float(&v, coord_val);
                        gst_value_array_append_value(&rect, &v);
                        GST_DEBUG_OBJECT(filter, "Rectangle[%d] = %.6f", j, coord_val);
                    }

                    gst_structure_set_value(bbox, "rectangle", &rect);
                    g_value_unset(&rect);
                    g_value_unset(&v);

                    /* Wrap bbox in a GValue */
                    GValue bbox_val = G_VALUE_INIT;
                    g_value_init(&bbox_val, GST_TYPE_STRUCTURE);
                    g_value_take_boxed(&bbox_val, bbox);

                    /* Append to bounding-boxes array */
                    gst_value_array_append_value(&bbox_array, &bbox_val);
                    g_value_unset(&bbox_val);
                    
                    GST_DEBUG_OBJECT(filter, "Added detection %zd: %s (%.2f)", 
                                   i, class_name, confidence);
                }
            }
        }
    } else {
        GST_WARNING_OBJECT(filter, "No 'detections' list found in dictionary");
    }

    /* -----------------------------
    * Insert bounding-boxes array
    * ----------------------------- */
    gst_structure_set_value(det, "bounding-boxes", &bbox_array);
    g_value_unset(&bbox_array);

    /* -----------------------------
    * Extract metadata from dictionary
    * ----------------------------- */
    PyObject *metadata_obj = PyDict_GetItemString(dict, "metadata");
    guint64 timestamp = 1835168501ULL; // Default fallback
    guint sequence_index = 1;
    guint sequence_num_entries = 1;
    
    if (metadata_obj && PyDict_Check(metadata_obj)) {
        PyObject *ts_obj = PyDict_GetItemString(metadata_obj, "timestamp");
        PyObject *seq_idx_obj = PyDict_GetItemString(metadata_obj, "sequence_index");
        PyObject *seq_num_obj = PyDict_GetItemString(metadata_obj, "sequence_num_entries");
        
        if (ts_obj && PyLong_Check(ts_obj)) {
            timestamp = (guint64)PyLong_AsLongLong(ts_obj);
        }
        if (seq_idx_obj && PyLong_Check(seq_idx_obj)) {
            sequence_index = (guint)PyLong_AsLong(seq_idx_obj);
        }
        if (seq_num_obj && PyLong_Check(seq_num_obj)) {
            sequence_num_entries = (guint)PyLong_AsLong(seq_num_obj);
        }
        
        GST_DEBUG_OBJECT(filter, "Extracted metadata: ts=%llu, seq_idx=%u, seq_num=%u", 
                       timestamp, sequence_index, sequence_num_entries);
    }
    
    gst_structure_set(det,
        "timestamp", G_TYPE_UINT64, timestamp,
        "sequence-index", G_TYPE_UINT, sequence_index,
        "sequence-num-entries", G_TYPE_UINT, sequence_num_entries,
        NULL
    );

    /* -----------------------------
    * Wrap structure in a GValue
    * ----------------------------- */
    GValue det_val = G_VALUE_INIT;
    g_value_init(&det_val, GST_TYPE_STRUCTURE);
    g_value_take_boxed(&det_val, det);

    /* Append to LIST */
    gst_value_list_append_value(&list, &det_val);
    g_value_unset(&det_val);

    /* -----------------------------
    * Serialize LIST (this is key!)
    * ----------------------------- */
    gchar *serialized = gst_value_serialize(&list);
    g_value_unset(&list);

    if (!serialized) {
        GST_ERROR_OBJECT(filter, "Failed to serialize dictionary structure");
        return FALSE;
    }

    GST_ERROR_OBJECT(filter, "Serialized structure length: %zu", strlen(serialized));
    GST_ERROR_OBJECT(filter, "Serialized content: %.200s...", serialized);

    GST_DEBUG_OBJECT(filter, "Serialized structure: %s", serialized);

    /* -----------------------------
    * Write into buffer (replace memory)
    * ----------------------------- */
    GstMemory *mem = gst_memory_new_wrapped(
        (GstMemoryFlags)0,
        serialized,
        strlen(serialized) + 1,
        0,
        strlen(serialized) + 1,
        serialized,
        g_free
    );

    /* Remove any existing memory in the buffer */
    gst_buffer_remove_all_memory(buf);

    /* Append ONLY our serialized text */
    gst_buffer_append_memory(buf, mem);

    GST_DEBUG_OBJECT(filter, "Dictionary-based Qualcomm-style structure written");
    return TRUE;
}


gboolean gst_tempy_process_numpy_array(GstTemPy *filter, PyObject *result, GstBuffer *buf) {
	if (!PyArray_Check(result)) {
		GST_ERROR_OBJECT(filter, "Python function must return a NumPy array");
		return FALSE;
	}
	
	PyArrayObject* array = (PyArrayObject*)result;
	
	// Ensure contiguous
	if (!PyArray_IS_C_CONTIGUOUS(array)) {
		array = (PyArrayObject*)PyArray_ContiguousFromAny(result, NPY_NOTYPE, 0, 0);
		if (!array) return FALSE;
	}
	
	// Get output format info
	GstCaps *outcaps = gst_pad_get_current_caps(GST_BASE_TRANSFORM_SRC_PAD(GST_BASE_TRANSFORM(filter)));
	GstStructure *structure = gst_caps_get_structure(outcaps, 0);
	const gchar *media_type = gst_structure_get_name(structure);
	
	// Map and copy data (universal - works for any format)
	GstMapInfo map;
	if (!gst_buffer_map(buf, &map, GST_MAP_WRITE)) {
		GST_ERROR_OBJECT(filter, "Failed to map output buffer");
		goto error;
	}
	
	gsize array_size = PyArray_NBYTES(array);
	if (array_size <= map.size) {
		memcpy(map.data, PyArray_DATA(array), array_size);
		GST_DEBUG_OBJECT(filter, "Copied %zu bytes to %s buffer", array_size, media_type);
	} else {
		GST_ERROR_OBJECT(filter, "Array too large: %zu bytes, buffer size: %zu", 
			array_size, map.size);
		gst_buffer_unmap(buf, &map);
		goto error;
	}
	
	gst_buffer_unmap(buf, &map);
	
	// Smart metadata handling
	if (g_str_has_prefix(media_type, "neural-network")) {
		// Add tensor metadata
		GstProtectionMeta *pmeta = gst_buffer_add_protection_meta(buf,
			gst_structure_new_empty(gst_batch_channel_name(0)));
		
		gst_structure_set(pmeta->info,
			"timestamp", G_TYPE_UINT64, GST_BUFFER_TIMESTAMP(buf),
			"sequence-index", G_TYPE_UINT, 1,
			"sequence-num-entries", G_TYPE_UINT, 1, NULL);
			
		GST_DEBUG_OBJECT(filter, "Added tensor metadata");
	}

	gst_caps_unref(outcaps);
	if (array != (PyArrayObject*)result) Py_DECREF(array);
	return TRUE;

error:
	gst_caps_unref(outcaps);
	if (array != (PyArrayObject*)result) Py_DECREF(array);
	return FALSE;
}



/*******************************************************************************
-------------------------------------------------------------------------------
  Helper Functions
-------------------------------------------------------------------------------
*******************************************************************************/
void gst_tempy_cleanup_python(GstTemPy * filter)
{
  if (filter->python_function) {
	  Py_DECREF(filter->python_function);
	  filter->python_function = NULL;
  }
  
  if (filter->python_module) {
	  Py_DECREF(filter->python_module);
	  filter->python_module = NULL;
  }
  
  filter->python_initialized = FALSE;
}
