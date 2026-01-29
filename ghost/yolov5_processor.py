import numpy as np
import json
import time

# ROS2 Jazzy imports
try:
    import rclpy
    from rclpy.node import Node
    from std_msgs.msg import Header
    from vision_msgs.msg import Detection2D, Detection2DArray, ObjectHypothesisWithPose
    ROS_AVAILABLE = True
    print("[YOLOv5] ROS2 imports successful")
except ImportError as e:
    print(f"[YOLOv5] ROS2 not available: {e}")
    ROS_AVAILABLE = False

# Global ROS2 variables
ros_node = None
ros_publisher = None
ros_initialized = False

def init_ros2_publisher():
    """Initialize ROS2 publisher for person detections"""
    global ros_node, ros_publisher, ros_initialized
    
    if not ROS_AVAILABLE or ros_initialized:
        return ros_initialized
        
    try:
        if not rclpy.ok():
            rclpy.init()
        
        ros_node = rclpy.create_node('yolov5_detector')
        ros_publisher = ros_node.create_publisher(
            Detection2DArray, 
            '/yolov5/person_detections', 
            10
        )
        
        ros_initialized = True
        print("[YOLOv5] ROS2 publisher initialized: /yolov5/person_detections")
        return True
        
    except Exception as e:
        print(f"[YOLOv5] ROS2 init failed: {e}")
        return False


def publish_person_detections(detections, labels):
    """
    Publish to ROS2 - Convert normalized to pixel coordinates HERE
    """
    global ros_node, ros_publisher
    
    if not ROS_AVAILABLE or ros_publisher is None:
        return
    
    try:
        detection_array = Detection2DArray()
        detection_array.header = Header()
        detection_array.header.stamp = ros_node.get_clock().now().to_msg()
        detection_array.header.frame_id = "camera_frame"
        
        # Camera resolution for coordinate conversion
        CAMERA_WIDTH = 1280
        CAMERA_HEIGHT = 720
        
        person_count = 0
        
        for detection in detections:
            class_id = detection['class_id']
            
            if class_id == 0 and class_id < len(labels) and labels[class_id] == 'person':
                
                det_msg = Detection2D()
                det_msg.header = detection_array.header
                
                # Convert normalized coordinates to pixel coordinates for ROS2
                bbox_norm = detection['bbox']  # [x_center, y_center, width, height] normalized
                
                # Convert to pixel coordinates
                x_center_pixel = bbox_norm[0] * CAMERA_WIDTH
                y_center_pixel = bbox_norm[1] * CAMERA_HEIGHT
                width_pixel = bbox_norm[2] * CAMERA_WIDTH
                height_pixel = bbox_norm[3] * CAMERA_HEIGHT
                
                det_msg.bbox.center.position.x = float(x_center_pixel)
                det_msg.bbox.center.position.y = float(y_center_pixel)
                det_msg.bbox.center.theta = 0.0
                det_msg.bbox.size_x = float(width_pixel)
                det_msg.bbox.size_y = float(height_pixel)
                
                det_msg.id = f"person_{detection.get('id', 256 + person_count)}"
                
                hypothesis = ObjectHypothesisWithPose()
                hypothesis.hypothesis.class_id = str(class_id)
                hypothesis.hypothesis.score = float(detection['confidence'])
                det_msg.results.append(hypothesis)
                
                detection_array.detections.append(det_msg)
                person_count += 1
                
                print(f"[YOLOv5] ROS2: Person at [{x_center_pixel:.1f},{y_center_pixel:.1f},{width_pixel:.1f},{height_pixel:.1f}] conf={detection['confidence']:.3f}")
        
        if person_count > 0:
            ros_publisher.publish(detection_array)
            print(f"[YOLOv5] Published {person_count} person(s) to ROS2")
        
        rclpy.spin_once(ros_node, timeout_sec=0.0)
        
    except Exception as e:
        print(f"[YOLOv5] ROS2 publish failed: {e}")
        

def yolov5_postprocess_text(tensor_info):
    """
    YOLOv5 post-processing with ROS2 person detection publishing
    """
    
    # Initialize ROS2 on first call
    if ROS_AVAILABLE:
        init_ros2_publisher()
    
    # Your existing processing code...
    tensor = tensor_info['data']
    print(f"[YOLOv5] Raw tensor: shape={tensor.shape}, dtype={tensor.dtype}")
    
    # Step 1: Convert raw bytes to FLOAT32 tensor
    try:
        if tensor.dtype == np.uint8:
            float32_data = tensor.view(np.float32)
        else:
            float32_data = tensor.astype(np.float32)
        
        expected_size = 1 * 6300 * 85
        
        if len(float32_data) >= expected_size:
            detections_raw = float32_data[:expected_size].reshape((1, 6300, 85))
        else:
            num_detections = len(float32_data) // 85
            detections_raw = float32_data[:num_detections * 85].reshape((1, num_detections, 85))
            
        print(f"[YOLOv5] Reshaped tensor: {detections_raw.shape}")
        
    except Exception as e:
        print(f"[YOLOv5] ERROR in tensor conversion: {e}")
        return create_error_dict(f"Tensor conversion failed: {e}")
    
    # Step 2: Parse detections with IMPROVED processing
    try:
        confidence_threshold = 0.3  # INCREASED threshold to reduce noise
        max_results = 3  # REDUCED to avoid clutter
        
        detections = parse_yolov5_detections(
            detections_raw, 
            confidence_threshold=confidence_threshold,
            max_results=max_results
        )
        
        print(f"[YOLOv5] Found {len(detections)} detections after NMS")
        
        # Debug: Print detection details
        for i, det in enumerate(detections):
            print(f"[YOLOv5] Detection {i}: bbox={det['bbox']}, conf={det['confidence']:.3f}, class_id={det['class_id']}")
        
    except Exception as e:
        print(f"[YOLOv5] ERROR in detection parsing: {e}")
        return create_error_dict(f"Detection parsing failed: {e}")
    
    # Step 3: Load class labels
    try:
        labels = load_yolov5_labels('/home/ubuntu/TFLite/yolov8.json')
        print(f"[YOLOv5] Labels loaded successfully: {len(labels)} classes")
    except Exception as e:
        print(f"[YOLOv5] Label loading failed: {e}")
        labels = get_coco_labels()
        print(f"[YOLOv5] Using fallback COCO labels: {len(labels)} classes")
    
    # NEW: Publish person detections to ROS2
    publish_person_detections(detections, labels)
    
    # Step 4: Your existing dictionary processing (unchanged)
    try:
        print(f"[YOLOv5] Starting dictionary conversion...")
        detection_list = []
        
        for i, det in enumerate(detections):
            class_id = det['class_id']
            confidence = det['confidence']
            bbox = det['bbox']
            
            if class_id < len(labels):
                class_name = str(labels[class_id]).replace(' ', '.').replace('_', '.')
            else:
                class_name = f"class.{class_id}"
            
            # Convert bbox format using the new function
            bbox_converted = convert_center_to_corner_bbox(
                bbox, 
                format_type="topleft", 
                flip_x=False, 
                flip_y=False,
                height_scale=1.2,  # 30% taller
                y_offset=0.1      # Move center down by 2%
            )

            detection_dict = {
                'class_name': class_name,
                'bbox': bbox_converted,
                'confidence': confidence,
                'class_id': class_id,
                'id': 256 + i
            }
            
            detection_list.append(detection_dict)
            print(f"[YOLOv5] Detection {i}: {class_name} ({confidence:.3f}) at {bbox_converted}")
        
        result_dict = {
            'type': 'detection',
            'detections': detection_list,
            'metadata': {
                'timestamp': int(time.time() * 1000000),
                'sequence_index': 1,
                'sequence_num_entries': 1,
                'model': 'yolov5m',
                'confidence_threshold': confidence_threshold,
                'max_results': max_results
            }
        }
        
        print(f"[YOLOv5] SUCCESS: Returning dictionary with {len(detection_list)} detections")
        return result_dict
        
    except Exception as e:
        print(f"[YOLOv5] ERROR in dictionary formatting: {e}")
        return create_error_dict(f"Dictionary formatting failed: {e}")


def apply_nms_normalized(detections, iou_threshold=0.45):
    """
    Apply NMS with normalized coordinates (0-1 range)
    """
    if len(detections) == 0:
        return []
    
    # Convert normalized center format to corner format for NMS
    boxes = []
    for det in detections:
        x_center, y_center, width, height = det['bbox']
        x1 = x_center - width / 2   # Left edge
        y1 = y_center - height / 2  # Top edge  
        x2 = x_center + width / 2   # Right edge
        y2 = y_center + height / 2  # Bottom edge
        
        boxes.append([x1, y1, x2, y2, det['confidence'], det['class_id']])
    
    boxes = np.array(boxes)
    
    # Apply NMS
    keep_indices = nms_boxes(boxes, iou_threshold)
    
    return [detections[i] for i in keep_indices]


def apply_nms(detections, iou_threshold=0.45):
    """
    Apply Non-Maximum Suppression to remove duplicate detections
    """
    if len(detections) == 0:
        return []
    
    # Convert to format needed for NMS: [x1, y1, x2, y2, confidence, class_id]
    boxes = []
    for det in detections:
        x, y, w, h = det['bbox']
        x1, y1 = x - w/2, y - h/2  # Convert center format to corner format
        x2, y2 = x + w/2, y + h/2
        boxes.append([x1, y1, x2, y2, det['confidence'], det['class_id']])
    
    boxes = np.array(boxes)
    
    # Apply NMS
    keep_indices = nms_boxes(boxes, iou_threshold)
    
    # Return filtered detections
    return [detections[i] for i in keep_indices]


def nms_boxes(boxes, iou_threshold):
    """
    Non-Maximum Suppression implementation
    """
    if len(boxes) == 0:
        return []
    
    # Sort by confidence (highest first)
    indices = np.argsort(boxes[:, 4])[::-1]
    keep = []
    
    while len(indices) > 0:
        # Keep the box with highest confidence
        current = indices[0]
        keep.append(current)
        
        if len(indices) == 1:
            break
            
        # Calculate IoU with remaining boxes
        current_box = boxes[current]
        remaining_boxes = boxes[indices[1:]]
        
        ious = calculate_iou(current_box[:4], remaining_boxes[:, :4])
        
        # Keep boxes with IoU below threshold
        indices = indices[1:][ious < iou_threshold]
    
    return keep


def calculate_iou(box1, boxes):
    """Calculate Intersection over Union"""
    x1_max = np.maximum(box1[0], boxes[:, 0])
    y1_max = np.maximum(box1[1], boxes[:, 1])
    x2_min = np.minimum(box1[2], boxes[:, 2])
    y2_min = np.minimum(box1[3], boxes[:, 3])
    
    intersection = np.maximum(0, x2_min - x1_max) * np.maximum(0, y2_min - y1_max)
    
    area1 = (box1[2] - box1[0]) * (box1[3] - box1[1])
    area2 = (boxes[:, 2] - boxes[:, 0]) * (boxes[:, 3] - boxes[:, 1])
    
    union = area1 + area2 - intersection
    
    return intersection / (union + 1e-6)


def create_error_dict(error_message):
    """
    Create error dictionary for C processing
    """
    return {
        'type': 'detection',
        'detections': [],
        'metadata': {
            'timestamp': int(time.time() * 1000000),
            'sequence_index': 1,
            'sequence_num_entries': 1,
            'error': error_message
        }
    }


def convert_yolo_to_proper_bbox(bbox, input_width=320, input_height=320, output_width=1280, output_height=720):
    """
    Convert YOLOv5 output coordinates to proper bounding box format
    
    YOLOv5 outputs normalized coordinates relative to model input size (320x320)
    Need to convert to actual camera resolution (1280x720)
    """
    x_center, y_center, width, height = bbox
    
    # YOLOv5 coordinates are normalized to model input size
    # Convert to actual pixel coordinates
    x_center_pixel = x_center * output_width
    y_center_pixel = y_center * output_height  
    width_pixel = width * output_width
    height_pixel = height * output_height
    
    # Return in center format (for ROS2) or convert to corner format
    return [x_center_pixel, y_center_pixel, width_pixel, height_pixel]


def parse_yolov5_detections(detections_raw, confidence_threshold=0.3, max_results=3):
    """
    Debug version - print raw coordinates to understand format
    """
    detections = []
    
    batch_size, num_anchors, num_values = detections_raw.shape
    num_classes = num_values - 5
    
    print(f"[YOLOv5] Processing {batch_size} batches, {num_anchors} anchors, {num_classes} classes")
    
    for batch_idx in range(batch_size):
        for anchor_idx in range(num_anchors):
            detection = detections_raw[batch_idx, anchor_idx]
            
            x, y, w, h = detection[0:4]
            objectness = detection[4]
            
            if objectness < 0.1:
                continue
                
            class_scores = detection[5:5+num_classes]
            class_id = np.argmax(class_scores)
            class_confidence = class_scores[class_id]
            final_confidence = objectness * class_confidence
            
            if final_confidence > confidence_threshold:
                # DEBUG: Print raw coordinates
                print(f"[YOLOv5] RAW COORDS: x={x:.3f}, y={y:.3f}, w={w:.3f}, h={h:.3f}")
                print(f"[YOLOv5] INTERPRETATION: center=({x:.3f},{y:.3f}), size=({w:.3f},{h:.3f})")
                
                # Calculate corner coordinates for debugging
                x1 = x - w/2  # Left edge
                y1 = y - h/2  # Top edge
                x2 = x + w/2  # Right edge
                y2 = y + h/2  # Bottom edge
                print(f"[YOLOv5] CORNERS: x1={x1:.3f}, y1={y1:.3f}, x2={x2:.3f}, y2={y2:.3f}")
                
                detections.append({
                    'bbox': [float(x), float(y), float(w), float(h)],  # Keep original format
                    'confidence': float(final_confidence),
                    'class_id': int(class_id),
                    'objectness': float(objectness),
                    'class_confidence': float(class_confidence)
                })
    
    # Apply NMS
    detections = apply_nms_normalized(detections, iou_threshold=0.45)
    
    detections.sort(key=lambda x: x['confidence'], reverse=True)
    return detections[:max_results]


def convert_center_to_corner_bbox(bbox, format_type="corner", flip_x=False, flip_y=False, 
                                 width_scale=1.0, height_scale=1.0, y_offset=0.0):
    """
    Convert center format bounding box with optional adjustments
    
    Args:
        bbox: [center_x, center_y, width, height] in normalized coordinates (0-1)
        format_type: "corner", "topleft", or "half_extents"
        flip_x: If True, flip X coordinates (1.0 - x)
        flip_y: If True, flip Y coordinates (1.0 - y)
        width_scale: Scale factor for width (1.0 = no change, 1.2 = 20% wider)
        height_scale: Scale factor for height (1.0 = no change, 1.3 = 30% taller)
        y_offset: Vertical offset for center position (positive = move down)
    
    Returns:
        Converted bounding box coordinates
    """
    center_x, center_y, width, height = bbox
    
    # Apply scaling to width and height
    width = width * width_scale
    height = height * height_scale
    
    # Apply vertical offset to center
    center_y = center_y + y_offset
    
    # Apply coordinate flipping if needed
    if flip_x:
        center_x = 1.0 - center_x
        print(f"[YOLOv5] X-FLIP: {bbox[0]:.3f} -> {center_x:.3f}")
    
    if flip_y:
        center_y = 1.0 - center_y
        print(f"[YOLOv5] Y-FLIP: {bbox[1]:.3f} -> {center_y:.3f}")
    
    # Show adjustments
    if width_scale != 1.0 or height_scale != 1.0 or y_offset != 0.0:
        print(f"[YOLOv5] ADJUSTMENTS: width_scale={width_scale}, height_scale={height_scale}, y_offset={y_offset}")
        print(f"[YOLOv5] ADJUSTED: size=({width:.3f},{height:.3f}), center_y={center_y:.3f}")
    
    if format_type == "corner":
        # Convert to corner format: [x1, y1, x2, y2]
        x1 = center_x - width / 2   # Left edge
        y1 = center_y - height / 2  # Top edge
        x2 = center_x + width / 2   # Right edge
        y2 = center_y + height / 2  # Bottom edge
        
        result = [x1, y1, x2, y2]
        print(f"[YOLOv5] CORNER: center=({center_x:.3f},{center_y:.3f}), size=({width:.3f},{height:.3f}) -> [{x1:.3f}, {y1:.3f}, {x2:.3f}, {y2:.3f}]")
        
    elif format_type == "topleft":
        # Convert to top-left + width/height: [x1, y1, width, height]
        x1 = center_x - width / 2
        y1 = center_y - height / 2
        
        result = [x1, y1, width, height]
        print(f"[YOLOv5] TOPLEFT: center=({center_x:.3f},{center_y:.3f}), size=({width:.3f},{height:.3f}) -> [{x1:.3f}, {y1:.3f}, {width:.3f}, {height:.3f}]")
        
    elif format_type == "half_extents":
        # Convert to center + half-extents: [center_x, center_y, half_width, half_height]
        half_width = width / 2
        half_height = height / 2
        
        result = [center_x, center_y, half_width, half_height]
        print(f"[YOLOv5] HALF_EXTENTS: center=({center_x:.3f},{center_y:.3f}), size=({width:.3f},{height:.3f}) -> [{center_x:.3f}, {center_y:.3f}, {half_width:.3f}, {half_height:.3f}]")
        
    else:
        # Keep original center format: [center_x, center_y, width, height]
        result = [center_x, center_y, width, height]
        print(f"[YOLOv5] CENTER: [{center_x:.3f}, {center_y:.3f}, {width:.3f}, {height:.3f}]")
    
    return result

def format_detections_for_metamux_structure(detections, labels, tensor_info):
    """
    Convert to corner format for qtimetamux
    """
    import time
    
    metamux_data = {
        'type': 'ObjectDetection',
        'bounding_boxes': [],
        'timestamp': int(time.time() * 1000000),
        'sequence_index': 0,
        'sequence_num_entries': 1
    }
    
    for i, detection in enumerate(detections):
        class_id = detection['class_id']
        confidence = detection['confidence']
        bbox = detection['bbox']  # [center_x, center_y, width, height]
        
        # Get class name
        if class_id < len(labels):
            class_name = labels[class_id].replace(' ', '.').replace('_', '.')
        else:
            class_name = f"class.{class_id}"
        
        # Convert center format to corner format
        center_x, center_y, width, height = bbox
        x1 = center_x - width / 2   # Left edge
        y1 = center_y - height / 2  # Top edge
        x2 = center_x + width / 2   # Right edge
        y2 = center_y + height / 2  # Bottom edge
        
        # Generate unique ID
        detection_id = (0 << 24) | (0 << 16) | i
        
        # Create bounding box structure with CORNER FORMAT
        bbox_structure = {
            'name': class_name,
            'id': detection_id,
            'confidence': float(confidence),
            'color': 0xFF00FF00,  # Green
            'rectangle': [
                float(x1),  # Left (x1)
                float(y1),  # Top (y1)  
                float(x2),  # Right (x2)
                float(y2)   # Bottom (y2)
            ]
        }
        
        metamux_data['bounding_boxes'].append(bbox_structure)
        
        print(f"[YOLOv5] CORNER FORMAT: [{x1:.3f}, {y1:.3f}, {x2:.3f}, {y2:.3f}] for {class_name}")
        
    print(f"[YOLOv5] Generated metamux structure with {len(metamux_data['bounding_boxes'])} detections")
    return metamux_data



def load_yolov5_labels(json_path):
    """
    Load YOLOv5/COCO class labels from JSON file
    """
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        print(f"[YOLOv5] JSON type: {type(data)}")
        
        # Handle different JSON structures
        if isinstance(data, list):
            # Check if it's a list of dictionaries (your format)
            if len(data) > 0 and isinstance(data[0], dict):
                # Extract labels from list of dicts: [{"id": 0, "label": "person"}, ...]
                labels = []
                for item in data:
                    if 'label' in item:
                        labels.append(item['label'])
                    elif 'name' in item:
                        labels.append(item['name'])
                    else:
                        labels.append(f"class_{item.get('id', len(labels))}")
                
                print(f"[YOLOv5] Extracted labels from list of dicts")
            else:
                # Simple list of strings: ["person", "bicycle", ...]
                labels = data
                print(f"[YOLOv5] Using simple list format")
                
        elif isinstance(data, dict):
            if 'labels' in data:
                labels = data['labels']
            elif 'names' in data:
                labels = data['names']
            else:
                # Assume it's a dict with id->name mapping: {"0": "person", "1": "bicycle"}
                labels = [data.get(str(i), f"class_{i}") for i in range(80)]
            
            print(f"[YOLOv5] Extracted labels from dict format")
        else:
            raise ValueError(f"Unexpected JSON structure: {type(data)}")
        
        print(f"[YOLOv5] Loaded {len(labels)} labels from {json_path}")
        print(f"[YOLOv5] First few labels: {labels[:5]}")
        return labels
        
    except Exception as e:
        raise RuntimeError(f"Failed to load labels from {json_path}: {e}")


def get_coco_labels():
    """
    Fallback COCO class names (80 classes)
    """
    return [
        'person', 'bicycle', 'car', 'motorcycle', 'airplane', 'bus', 'train', 'truck',
        'boat', 'traffic light', 'fire hydrant', 'stop sign', 'parking meter', 'bench',
        'bird', 'cat', 'dog', 'horse', 'sheep', 'cow', 'elephant', 'bear', 'zebra',
        'giraffe', 'backpack', 'umbrella', 'handbag', 'tie', 'suitcase', 'frisbee',
        'skis', 'snowboard', 'sports ball', 'kite', 'baseball bat', 'baseball glove',
        'skateboard', 'surfboard', 'tennis racket', 'bottle', 'wine glass', 'cup',
        'fork', 'knife', 'spoon', 'bowl', 'banana', 'apple', 'sandwich', 'orange',
        'broccoli', 'carrot', 'hot dog', 'pizza', 'donut', 'cake', 'chair', 'couch',
        'potted plant', 'bed', 'dining table', 'toilet', 'tv', 'laptop', 'mouse',
        'remote', 'keyboard', 'cell phone', 'microwave', 'oven', 'toaster', 'sink',
        'refrigerator', 'book', 'clock', 'vase', 'scissors', 'teddy bear', 'hair drier',
        'toothbrush'
    ]

