#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Disable deprecated NumPy API warnings
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include "qtighost.hpp"
#include <gst/video/video.h>
#include <Python.h>
#include <numpy/arrayobject.h>

GST_DEBUG_CATEGORY_STATIC(gst_qtighost_debug);
#define GST_CAT_DEFAULT gst_qtighost_debug

// Properties
enum {
    PROP_0,
    PROP_PYTHON_SCRIPT,
    PROP_PYTHON_FUNCTION
};

// Default values
#define DEFAULT_PYTHON_SCRIPT ""
#define DEFAULT_PYTHON_FUNCTION "process_frame"

// Pad capabilities - supports common video formats
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, "
        "format=(string){RGB,BGR}, "
        "width=(int)[1,MAX], "
        "height=(int)[1,MAX], "
        "framerate=(fraction)[0/1,MAX]"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, "
        "format=(string){RGB,BGR}, "
        "width=(int)[1,MAX], "
        "height=(int)[1,MAX], "
        "framerate=(fraction)[0/1,MAX]"));

// Function declarations
static void gst_qtighost_class_init(GstQtiGhostClass * klass);
static void gst_qtighost_init(GstQtiGhost * filter);
static void gst_qtighost_finalize(GObject * object);
static void gst_qtighost_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_qtighost_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_qtighost_set_caps(GstBaseTransform * trans, GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_qtighost_transform_ip(GstBaseTransform * trans, GstBuffer * buf);
static gboolean gst_qtighost_start(GstBaseTransform * trans);
static gboolean gst_qtighost_stop(GstBaseTransform * trans);

// Python helper functions
static gboolean gst_qtighost_init_python(GstQtiGhost * filter);
static void gst_qtighost_cleanup_python(GstQtiGhost * filter);
static PyObject* gst_qtighost_buffer_to_numpy(GstQtiGhost * filter, GstBuffer * buf);
static gboolean gst_qtighost_numpy_to_buffer(GstQtiGhost * filter, PyObject * array, GstBuffer * buf);

// Global flag for NumPy initialization
static gboolean numpy_initialized = FALSE;

// Define the type
#define gst_qtighost_parent_class parent_class
G_DEFINE_TYPE(GstQtiGhost, gst_qtighost, GST_TYPE_BASE_TRANSFORM);

// Class initialization
static void gst_qtighost_class_init(GstQtiGhostClass * klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass *transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    // Set finalize, property functions
    gobject_class->finalize = gst_qtighost_finalize;
    gobject_class->set_property = gst_qtighost_set_property;
    gobject_class->get_property = gst_qtighost_get_property;

    // Install properties
    g_object_class_install_property(gobject_class, PROP_PYTHON_SCRIPT,
        g_param_spec_string("script", "Python Script", 
            "Path to Python script containing processing function",
            DEFAULT_PYTHON_SCRIPT, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_PYTHON_FUNCTION,
        g_param_spec_string("function", "Python Function", 
            "Name of Python function to call for processing",
            DEFAULT_PYTHON_FUNCTION, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    // Set element metadata
    gst_element_class_set_static_metadata(gstelement_class,
        "QTI Ghost Python Filter",
        "Filter/Effect/Video",
        "Video filter with Python callback for OpenCV processing",
        "Your Name <your.email@example.com>");

    // Add pad templates
    gst_element_class_add_static_pad_template(gstelement_class, &src_factory);
    gst_element_class_add_static_pad_template(gstelement_class, &sink_factory);

    // Set transform functions
    transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_qtighost_set_caps);
    transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_qtighost_transform_ip);
    transform_class->start = GST_DEBUG_FUNCPTR(gst_qtighost_start);
    transform_class->stop = GST_DEBUG_FUNCPTR(gst_qtighost_stop);
}

// Instance initialization
static void gst_qtighost_init(GstQtiGhost * filter)
{
    filter->frame_count = 0;
    filter->python_module = NULL;
    filter->python_function = NULL;
    filter->python_initialized = FALSE;
    filter->python_script_path = g_strdup(DEFAULT_PYTHON_SCRIPT);
    filter->python_function_name = g_strdup(DEFAULT_PYTHON_FUNCTION);
    
    gst_video_info_init(&filter->video_info);
}

static void gst_qtighost_finalize(GObject * object)
{
    GstQtiGhost *filter = GST_QTIGHOST(object);
    
    gst_qtighost_cleanup_python(filter);
    g_free(filter->python_script_path);
    g_free(filter->python_function_name);
    
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_qtighost_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    GstQtiGhost *filter = GST_QTIGHOST(object);
    
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

static void gst_qtighost_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
    GstQtiGhost *filter = GST_QTIGHOST(object);
    
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

static gboolean gst_qtighost_start(GstBaseTransform * trans)
{
    GstQtiGhost *filter = GST_QTIGHOST(trans);
    return gst_qtighost_init_python(filter);
}

static gboolean gst_qtighost_stop(GstBaseTransform * trans)
{
    GstQtiGhost *filter = GST_QTIGHOST(trans);
    gst_qtighost_cleanup_python(filter);
    return TRUE;
}

static gboolean gst_qtighost_set_caps(GstBaseTransform * trans, GstCaps * incaps, GstCaps * outcaps)
{
    GstQtiGhost *filter = GST_QTIGHOST(trans);
    
    if (!gst_video_info_from_caps(&filter->video_info, incaps)) {
        GST_ERROR_OBJECT(filter, "Failed to parse input caps");
        return FALSE;
    }
    
    GST_INFO_OBJECT(filter, "Set caps: %dx%d, format: %s", 
        GST_VIDEO_INFO_WIDTH(&filter->video_info),
        GST_VIDEO_INFO_HEIGHT(&filter->video_info),
        gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&filter->video_info)));
    
    return TRUE;
}

// Helper function for NumPy initialization (returns void* as expected by import_array)
static void* init_numpy_helper() {
    import_array();
    return NULL;
}

// Also update the initialization to enable threading
static gboolean gst_qtighost_init_python(GstQtiGhost * filter)
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

static void gst_qtighost_cleanup_python(GstQtiGhost * filter)
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

// Convert GStreamer buffer to NumPy array
static PyObject* gst_qtighost_buffer_to_numpy(GstQtiGhost * filter, GstBuffer * buf)
{
    GstMapInfo map;
    
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(filter, "Failed to map buffer");
        return NULL;
    }
    
    // Get video dimensions
    gint width = GST_VIDEO_INFO_WIDTH(&filter->video_info);
    gint height = GST_VIDEO_INFO_HEIGHT(&filter->video_info);
    
    // Calculate expected size based on format
    GstVideoFormat format = GST_VIDEO_INFO_FORMAT(&filter->video_info);
    gint channels;
    gint bytes_per_pixel;
    
    switch (format) {
        case GST_VIDEO_FORMAT_RGB:
        case GST_VIDEO_FORMAT_BGR:
            channels = 3;
            bytes_per_pixel = 3;
            break;
        case GST_VIDEO_FORMAT_RGBA:
        case GST_VIDEO_FORMAT_BGRA:
            channels = 4;
            bytes_per_pixel = 4;
            break;
        default:
            GST_ERROR_OBJECT(filter, "Unsupported video format: %s", 
                gst_video_format_to_string(format));
            gst_buffer_unmap(buf, &map);
            return NULL;
    }
    
    // Verify buffer size
    gsize expected_size = width * height * bytes_per_pixel;
    if (map.size != expected_size) {
        GST_WARNING_OBJECT(filter, "Buffer size mismatch: got %zu, expected %zu", 
            map.size, expected_size);
    }
    
    // Create NumPy array dimensions
    npy_intp dims[3] = {height, width, channels};
    
    // Create NumPy array and copy data
    PyObject* array = PyArray_SimpleNew(3, dims, NPY_UINT8);
    if (!array) {
        GST_ERROR_OBJECT(filter, "Failed to create NumPy array");
        gst_buffer_unmap(buf, &map);
        return NULL;
    }
    
    // Get pointer to array data
    unsigned char* array_data = (unsigned char*)PyArray_DATA((PyArrayObject*)array);
    
    // Copy data from GStreamer buffer to NumPy array
    memcpy(array_data, map.data, MIN(map.size, expected_size));
    
    gst_buffer_unmap(buf, &map);
    
    GST_DEBUG_OBJECT(filter, "Created NumPy array: %dx%dx%d", height, width, channels);
    
    return array;
}

// Convert NumPy array back to GStreamer buffer
static gboolean gst_qtighost_numpy_to_buffer(GstQtiGhost * filter, PyObject * result, GstBuffer * buf)
{
    // Check if result is a NumPy array
    if (!PyArray_Check(result)) {
        GST_ERROR_OBJECT(filter, "Python function must return a NumPy array");
        return FALSE;
    }
    
    PyArrayObject* array = (PyArrayObject*)result;
    
    // Ensure array is contiguous and has the right type
    if (!PyArray_IS_C_CONTIGUOUS(array)) {
        GST_WARNING_OBJECT(filter, "Array is not C-contiguous, making contiguous copy");
        array = (PyArrayObject*)PyArray_ContiguousFromAny(result, NPY_UINT8, 0, 0);
        if (!array) {
            GST_ERROR_OBJECT(filter, "Failed to make array contiguous");
            return FALSE;
        }
    }
    
    // Check array dimensions
    int ndim = PyArray_NDIM(array);
    if (ndim != 3) {
        GST_ERROR_OBJECT(filter, "Expected 3D array (height, width, channels), got %dD", ndim);
        if (array != (PyArrayObject*)result) {
            Py_DECREF(array);
        }
        return FALSE;
    }
    
    npy_intp* shape = PyArray_SHAPE(array);
    gint height = shape[0];
    gint width = shape[1];
    gint channels = shape[2];
    
    // Verify dimensions match
    if (width != GST_VIDEO_INFO_WIDTH(&filter->video_info) ||
        height != GST_VIDEO_INFO_HEIGHT(&filter->video_info)) {
        GST_ERROR_OBJECT(filter, "Array dimensions mismatch: got %dx%d, expected %dx%d",
            width, height,
            GST_VIDEO_INFO_WIDTH(&filter->video_info),
            GST_VIDEO_INFO_HEIGHT(&filter->video_info));
        if (array != (PyArrayObject*)result) {
            Py_DECREF(array);
        }
        return FALSE;
    }
    
    // Map output buffer
    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_WRITE)) {
        GST_ERROR_OBJECT(filter, "Failed to map output buffer");
        if (array != (PyArrayObject*)result) {
            Py_DECREF(array);
        }
        return FALSE;
    }
    
    // Get array data
    unsigned char* array_data = (unsigned char*)PyArray_DATA(array);
    gsize array_size = PyArray_NBYTES(array);
    
    // Copy data from NumPy array to buffer
    if (array_size <= map.size) {
        memcpy(map.data, array_data, array_size);
        GST_DEBUG_OBJECT(filter, "Copied %zu bytes from NumPy array to buffer", array_size);
    } else {
        GST_ERROR_OBJECT(filter, "Array too large: %zu bytes, buffer size: %zu", 
            array_size, map.size);
        gst_buffer_unmap(buf, &map);
        if (array != (PyArrayObject*)result) {
            Py_DECREF(array);
        }
        return FALSE;
    }
    
    gst_buffer_unmap(buf, &map);
    
    // Clean up if we made a contiguous copy
    if (array != (PyArrayObject*)result) {
        Py_DECREF(array);
    }
    
    return TRUE;
}

// The main transform function with better error handling
static GstFlowReturn gst_qtighost_transform_ip(GstBaseTransform * trans, GstBuffer * buf)
{
    GstQtiGhost *filter = GST_QTIGHOST(trans);
    
    filter->frame_count++;
    
    // If no Python function is loaded, just pass through
    if (!filter->python_function) {
        GST_DEBUG_OBJECT(filter, "Frame #%u passed through (no Python function)", filter->frame_count);
        return GST_FLOW_OK;
    }
    
    // Acquire GIL (Global Interpreter Lock) for thread safety
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    
    // Convert buffer to NumPy array
    PyObject* input_array = gst_qtighost_buffer_to_numpy(filter, buf);
    if (!input_array) {
        GST_ERROR_OBJECT(filter, "Failed to convert buffer to NumPy array");
        PyGILState_Release(gstate);
        return GST_FLOW_ERROR;
    }
    
    // Call Python function
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, input_array); // steals reference
    
    PyObject* result = PyObject_CallObject(filter->python_function, args);
    Py_DECREF(args);
    
    if (!result) {
        GST_ERROR_OBJECT(filter, "Python function call failed");
        PyErr_Print();
        PyGILState_Release(gstate);
        return GST_FLOW_ERROR;
    }
    
    // Convert result back to buffer
    gboolean success = gst_qtighost_numpy_to_buffer(filter, result, buf);
    Py_DECREF(result);
    
    // Release GIL
    PyGILState_Release(gstate);
    
    if (!success) {
        return GST_FLOW_ERROR;
    }
    
    GST_DEBUG_OBJECT(filter, "Frame #%u processed successfully", filter->frame_count);
    return GST_FLOW_OK;
}

// Plugin initialization function
static gboolean plugin_init(GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_qtighost_debug, "qtighost", 0, "QTI Ghost Python filter");
    return gst_element_register(plugin, "qtighost", GST_RANK_NONE, GST_TYPE_QTIGHOST);
}

// Plugin definition macro
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    qtighost,
    "QTI Ghost Python callback video filter",
    plugin_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN)