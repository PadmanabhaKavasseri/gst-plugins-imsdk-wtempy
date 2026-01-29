1. qtimltflite says: "I need neural-network/tensors,dimensions=<1,224,224,3>"
2. GStreamer asks qtitempy: "What input caps do you need for that output?"
3. qtitempy transform_caps() returns: "video/x-raw,format=NV12,width=[1,224],height=[1,224]"
4. GStreamer asks decoder: "Can you produce video/x-raw,format=NV12?"
5. Decoder says: "Yes, I can produce video/x-raw,format=NV12,width=1920,height=1080"
6. GStreamer negotiates: NV12 1920x1080 → qtitempy → tensor 1,224,224,3


GstCaps* gst_tempy_transform_caps(GstBaseTransform *base, 
                                  GstPadDirection direction,
                                  GstCaps *caps, GstCaps *filter) {
    
    GstCaps *result;
    
    if (direction == GST_PAD_SINK) {
        // "Given any input, I can produce these specific output types"
        result = gst_caps_new_empty();
        
        // Add video output capability
        gst_caps_append(result, gst_caps_new_simple("video/x-raw",
            "format", GST_TYPE_LIST, /* supported formats */,
            "width", GST_TYPE_INT_RANGE, 1, 4096,
            "height", GST_TYPE_INT_RANGE, 1, 4096,
            NULL));
            
        // Add tensor output capability  
        gst_caps_append(result, gst_caps_new_simple("neural-network/tensors",
            "type", GST_TYPE_LIST, /* uint8, float32 */,
            NULL));
            
    } else {
        // "To produce any output, I can accept these input types"
        result = gst_caps_new_empty();
        
        // Add video input capability
        gst_caps_append(result, gst_caps_new_simple("video/x-raw",
            "format", GST_TYPE_LIST, /* supported formats */,
            "width", GST_TYPE_INT_RANGE, 1, 4096, 
            "height", GST_TYPE_INT_RANGE, 1, 4096,
            NULL));
            
        // Add tensor input capability
        gst_caps_append(result, gst_caps_new_simple("neural-network/tensors",
            "type", GST_TYPE_LIST, /* uint8, float32 */,
            NULL));
    }
    
    // Apply filter if provided
    if (filter) {
        GstCaps *intersection = gst_caps_intersect_full(result, filter, 
                                                        GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(result);
        result = intersection;
    }
    
    return result;
}
