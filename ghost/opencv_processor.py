import cv2
import numpy as np

def gray_scale_filter(frame_info):
    """
    Simple grayscale filter for video→video processing
    """
    
    # Extract data and format information
    frame = frame_info['data']
    
    # Get output dimensions (input dimensions are in the frame data)
    output_width = frame_info.get('output_width', frame_info.get('input_width', 320))
    output_height = frame_info.get('output_height', frame_info.get('input_height', 240))
    
    print(f"Processing frame: {frame.shape}, dtype: {frame.dtype}")
    print(f"Output size: {output_width}x{output_height}")
    
    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
    
    # Convert back to 3-channel format (RGB)
    gray_rgb = cv2.cvtColor(gray, cv2.COLOR_GRAY2RGB)
    
    # Resize if output dimensions are different
    if gray_rgb.shape[:2] != (output_height, output_width):
        result = cv2.resize(gray_rgb, (output_width, output_height))
    else:
        result = gray_rgb
    
    print(f"Result: {result.shape}, dtype: {result.dtype}")
    return result

def mobilenet_preprocess(frame_info):
    """
    Preprocess for MobileNet v2 quantized model
    
    Expected input: NV12 video from v4l2h264dec
    Output: [1, 224, 224, 3] UINT8 tensor for quantized MobileNet
    """
    # Extract frame data and format information
    frame = frame_info['data']
    
    # Get input format info
    input_type = frame_info.get('input_type', 'unknown')
    input_width = frame_info.get('input_width', 0)
    input_height = frame_info.get('input_height', 0)
    input_format = frame_info.get('input_format', 'unknown')
    
    print(f"Input: {input_type} {input_width}x{input_height} {input_format}")
    print(f"Frame data: {frame.shape}, dtype: {frame.dtype}")
    
    # Validate expected format
    if input_type != 'video':
        raise ValueError(f"Expected video input, got: {input_type}")
    
    if input_format != 'NV12':
        raise ValueError(f"Expected NV12 format, got: {input_format}")
    
    if len(frame.shape) != 1:
        raise ValueError(f"Expected 1D NV12 buffer, got shape: {frame.shape}")
    
    # Convert NV12 to RGB
    try:
        # Calculate expected NV12 size
        y_size = input_width * input_height
        uv_size = input_width * input_height // 2
        expected_total = y_size + uv_size
        
        print(f"Processing NV12: {input_width}x{input_height}, buffer size: {len(frame)}")
        print(f"Expected NV12 size: {expected_total}, actual: {len(frame)}")
        
        if len(frame) < expected_total:
            raise ValueError(f"Buffer too small: got {len(frame)}, need {expected_total}")
        
        # Reshape and convert NV12 to RGB
        nv12_data = frame[:expected_total].reshape((input_height * 3 // 2, input_width))
        rgb_frame = cv2.cvtColor(nv12_data, cv2.COLOR_YUV2RGB_NV12)
        print(f"NV12 conversion successful: {rgb_frame.shape}")
        
    except Exception as e:
        raise RuntimeError(f"NV12 to RGB conversion failed: {e}")
    
    # Resize to MobileNet input size (224x224)
    if rgb_frame.shape[:2] != (224, 224):
        resized = cv2.resize(rgb_frame, (224, 224), interpolation=cv2.INTER_LINEAR)
    else:
        resized = rgb_frame.copy()
    
    # Apply custom preprocessing
    # processed = apply_custom_preprocessing(resized)
    processed = resized
    
    # Ensure UINT8 format for quantized model
    if processed.dtype != np.uint8:
        processed = np.clip(processed, 0, 255).astype(np.uint8)
    
    # Add batch dimension: [224, 224, 3] → [1, 224, 224, 3]
    tensor = np.expand_dims(processed, axis=0)
    
    print(f"Output tensor: {tensor.shape}, dtype: {tensor.dtype}")
    print(f"Tensor range: [{tensor.min()}, {tensor.max()}]")
    
    return tensor

def load_imagenet_labels():
    """
    Load ImageNet class labels from JSON file
    """
    import json
    
    json_path = '/home/ubuntu/TFLite/mobilenet_old.json'  # Update this path
    
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        # Extract labels from JSON array, sorted by ID
        labels_dict = {}
        for item in data:
            labels_dict[item['id']] = item['label']
        
        # Create ordered list (ID 0 to max_id)
        max_id = max(labels_dict.keys())
        labels = []
        for i in range(max_id + 1):
            if i in labels_dict:
                labels.append(labels_dict[i])
            else:
                labels.append(f"unknown_class_{i}")
        
        print(f"Loaded {len(labels)} labels from {json_path}")
        print(f"ID range: 0 to {max_id}")
        print(f"Sample labels: {labels[:5]}")
        
        return labels
        
    except FileNotFoundError:
        raise FileNotFoundError(f"JSON labels file not found: {json_path}")
    except Exception as e:
        raise RuntimeError(f"Failed to load JSON labels from {json_path}: {e}")

def create_classification_overlay(top_indices, top_scores, labels, width, height):
    """
    Create a rich video frame with classification results overlay
    """

    print(f"DEBUG: labels loaded: {len(labels)}")
    print(f"DEBUG: top_indices: {top_indices}")
    print(f"DEBUG: max index: {max(top_indices)}")


    # Create BGRA background
    frame = np.zeros((height, width, 4), dtype=np.uint8)
    frame[:, :, 3] = 255  # Alpha channel = opaque
    
    # Set gradient background (dark blue to black)
    for y in range(height):
        intensity = int(50 * (1 - y / height))
        frame[y, :, 0] = intensity  # Blue
        frame[y, :, 1] = intensity // 2  # Green  
        frame[y, :, 2] = intensity // 3  # Red
    
    # Make sure array is contiguous for OpenCV
    frame = np.ascontiguousarray(frame)
    
    # Font settings
    font = cv2.FONT_HERSHEY_SIMPLEX
    title_font_scale = 0.8
    text_font_scale = 0.6
    thickness = 2
    
    # Title
    title = "MobileNet Classification"
    title_size = cv2.getTextSize(title, font, title_font_scale, thickness)[0]
    title_x = (width - title_size[0]) // 2
    cv2.putText(frame, title, (title_x, 40), font, title_font_scale, 
                (255, 255, 255, 255), thickness)  # White text in BGRA
    
    # Draw top predictions
    y_start = 80
    bar_width_max = min(200, width - 300)  # Max width for confidence bars
    
    for i, (idx, score) in enumerate(zip(top_indices, top_scores)):
        if idx < len(labels):
            label = labels[idx]
            # Truncate long labels
            if len(label) > 20:
                label = label[:17] + "..."
        else:
            label = f"Class {idx}"
            
        confidence = float(score) * 100
        
        # Color based on confidence (green = high, yellow = medium, red = low)
        if confidence > 70:
            color = (0, 255, 0, 255)  # Green in BGRA
        elif confidence > 40:
            color = (0, 255, 255, 255)  # Yellow in BGRA
        else:
            color = (0, 0, 255, 255)  # Red in BGRA
        
        # Position for this prediction
        y_pos = y_start + i * 45
        
        # Rank number
        rank_text = f"{i+1}."
        cv2.putText(frame, rank_text, (20, y_pos), font, text_font_scale, 
                   (200, 200, 200, 255), 2)  # Light gray
        
        # Class label
        cv2.putText(frame, label, (50, y_pos), font, text_font_scale, 
                   (255, 255, 255, 255), 2)  # White
        
        # Confidence percentage
        conf_text = f"{confidence:.1f}%"
        cv2.putText(frame, conf_text, (250, y_pos), font, text_font_scale, 
                   color, 2)
        
        # Confidence bar
        bar_width = int((confidence / 100.0) * bar_width_max)
        bar_start_x = 320
        bar_y_top = y_pos - 15
        bar_y_bottom = y_pos - 5
        
        # Background bar (dark)
        cv2.rectangle(frame, (bar_start_x, bar_y_top), 
                     (bar_start_x + bar_width_max, bar_y_bottom), 
                     (30, 30, 30, 255), -1)
        
        # Confidence bar (colored)
        if bar_width > 0:
            cv2.rectangle(frame, (bar_start_x, bar_y_top), 
                         (bar_start_x + bar_width, bar_y_bottom), 
                         color, -1)
    
    # Add footer info
    footer_y = height - 20
    timestamp = f"Frame processed - Top class: {top_indices[0]}"
    cv2.putText(frame, timestamp, (20, footer_y), font, 0.4, 
               (150, 150, 150, 255), 1)  # Gray text
    
    return frame

def get_top5_predictions_numpy(predictions):
    """
    NumPy equivalent of torch.topk(predictions, 5)
    """
    # Get top 5 indices (highest to lowest)
    top5_indices = np.argsort(predictions)[-5:][::-1]
    
    # Get corresponding values
    top5_values = predictions[top5_indices]
    
    return top5_values, top5_indices

def mobilenet_postprocess_top1(tensor_info):
    """
    MobileNet postprocessing: Raw bytes → FLOAT32 → Softmax → Top1
    
    Expected: 4004 UINT8 bytes (actually 1001 FLOAT32 logits)
    Output: BGRA video frame with top1 classification
    """
    # Extract tensor data
    tensor = tensor_info['data']
    output_width = tensor_info.get('output_width', 640)
    output_height = tensor_info.get('output_height', 360)
    
    print(f"Raw tensor: shape={tensor.shape}, dtype={tensor.dtype}")
    
    # Step 1: Convert raw UINT8 bytes to FLOAT32 logits
    try:
        # Reinterpret 4004 UINT8 bytes as 1001 FLOAT32 values
        float32_data = tensor.view(np.float32)
        
        # Take first 1001 values (MobileNet output size)
        if len(float32_data) >= 1001:
            logits = float32_data[:1001]
        else:
            logits = float32_data
            
        print(f"Logits: shape={logits.shape}, range=[{logits.min():.3f}, {logits.max():.3f}]")
        
    except Exception as e:
        raise RuntimeError(f"Failed to convert raw bytes to FLOAT32: {e}")
    
    # Step 2: Apply softmax to convert logits to probabilities
    try:
        probabilities = softmax(logits)
        print(f"Probabilities: range=[{probabilities.min():.6f}, {probabilities.max():.6f}]")
        print(f"Probabilities sum: {probabilities.sum():.6f}")
        
    except Exception as e:
        raise RuntimeError(f"Softmax failed: {e}")
    
    # Step 3: Get top1 prediction
    try:
        top1_index = np.argmax(probabilities)
        top1_confidence = probabilities[top1_index]
        
        print(f"Top1: index={top1_index}, confidence={top1_confidence:.6f} ({top1_confidence*100:.2f}%)")
        
    except Exception as e:
        raise RuntimeError(f"Top1 extraction failed: {e}")
    
    # Step 4: Get label name
    try:
        labels = load_imagenet_labels()
        
        if top1_index < len(labels):
            label_name = labels[top1_index]
        else:
            label_name = f"Class {top1_index}"
        
        print(f"Top1 prediction: {label_name} ({top1_confidence*100:.1f}%)")
        
    except Exception as e:
        print(f"Label loading failed: {e}")
        label_name = f"Class {top1_index}"
    
    # Step 5: Create output frame
    try:
        frame = np.zeros((output_height, output_width, 4), dtype=np.uint8)
        frame[:, :, 3] = 255  # Alpha
        frame[:, :, 0] = 20   # Dark background
        frame[:, :, 1] = 20
        frame[:, :, 2] = 20
        
        frame = np.ascontiguousarray(frame)
        
        # Clean up label for display
        display_label = label_name.replace('_', ' ').title()
        confidence_pct = top1_confidence * 100
        
        # Text settings
        font = cv2.FONT_HERSHEY_SIMPLEX
        main_text = f"{display_label}"
        conf_text = f"{confidence_pct:.1f}%"
        
        # Color based on confidence
        if confidence_pct > 80:
            color = (0, 255, 0, 255)    # Green - high confidence
        elif confidence_pct > 50:
            color = (0, 255, 255, 255)  # Yellow - medium confidence
        else:
            color = (0, 100, 255, 255)  # Orange - low confidence
        
        # Center text
        main_size = cv2.getTextSize(main_text, font, 1.0, 2)[0]
        conf_size = cv2.getTextSize(conf_text, font, 0.8, 2)[0]
        
        main_x = (output_width - main_size[0]) // 2
        main_y = output_height // 2 - 20
        conf_x = (output_width - conf_size[0]) // 2
        conf_y = output_height // 2 + 30
        
        # Draw text with shadow for better visibility
        shadow_offset = 2
        shadow_color = (0, 0, 0, 255)  # Black shadow
        
        # Shadow
        cv2.putText(frame, main_text, (main_x + shadow_offset, main_y + shadow_offset), 
                   font, 1.0, shadow_color, 2)
        cv2.putText(frame, conf_text, (conf_x + shadow_offset, conf_y + shadow_offset), 
                   font, 0.8, shadow_color, 2)
        
        # Main text
        cv2.putText(frame, main_text, (main_x, main_y), font, 1.0, color, 2)
        cv2.putText(frame, conf_text, (conf_x, conf_y), font, 0.8, color, 2)
        
        return frame
        
    except Exception as e:
        print(f"Frame creation failed: {e}")
        emergency_frame = np.zeros((output_height, output_width, 4), dtype=np.uint8)
        emergency_frame[:, :, 3] = 255
        return emergency_frame

def softmax(x):
    """
    NumPy softmax implementation with numerical stability
    """
    # Subtract max for numerical stability
    x_shifted = x - np.max(x)
    exp_x = np.exp(x_shifted)
    return exp_x / np.sum(exp_x)

