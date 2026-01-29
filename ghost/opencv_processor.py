import cv2
import numpy as np

def process_frame(frame):
    """
    Process a video frame using OpenCV.
    
    Args:
        frame: NumPy array of shape (height, width, 3) in RGB or BGR format
        
    Returns:
        NumPy array of the same shape with processed image
    """
    print(f"Processing frame: {frame.shape}, dtype: {frame.dtype}")
    
    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
    
    # Convert back to 3-channel format (RGB)
    # This maintains compatibility with the GStreamer pipeline
    result = cv2.cvtColor(gray, cv2.COLOR_GRAY2RGB)
    
    return result

def process_frame_edge_detection(frame):
    """
    Alternative processing function that applies edge detection.
    """
    print(f"Edge detection on frame: {frame.shape}")
    
    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
    
    # Apply Canny edge detection
    edges = cv2.Canny(gray, 100, 200)
    
    # Convert back to 3-channel RGB
    result = cv2.cvtColor(edges, cv2.COLOR_GRAY2RGB)
    
    return result

def process_frame_blur(frame):
    """
    Alternative processing function that applies Gaussian blur.
    """
    print(f"Blurring frame: {frame.shape}")
    
    # Apply Gaussian blur
    result = cv2.GaussianBlur(frame, (15, 15), 0)
    
    return result