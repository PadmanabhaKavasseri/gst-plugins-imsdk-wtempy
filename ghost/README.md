# GST-TEMPLATE (GHOST) PLUGIN

### Install Dependencies
```
sudo apt install python3-dev python3-numpy python3-opencv-python python3-numpy-dev
```

### Build Insructions
```
cd /home/ubuntu/Projects/gst-template-plugin
mkdir build && cd build
cmake ..
make
sudo make install
```

### Export GST Plugin Path 
```
export GST_PLUGIN_PATH=$PWD:$GST_PLUGIN_PATH
```

### Grey Scale Pipeline CMD
```
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 videotestsrc ! videoconvert ! qtighost script=./opencv_processor.py function=process_frame ! videoconvert ! autovideosink
```
---------------

### Original Pipeline
```
gst-launch-1.0 -e --gst-debug=2 filesrc location=/opt/data/Animals_000_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<1920,1080>" ! queue ! waylandsink sync=false fullscreen=true \
split. ! queue ! qtimlvconverter ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/opt/data/mobilenet_v2-mobilenet-v2-float.tflite ! queue ! qtimlpostprocess results=1 module=mobilenet labels=/opt/data/mobilenet_old.json settings="{\"confidence\": 51.0}" ! video/x-raw,format=BGRA,width=640,height=360 ! queue ! mixer.

```
### Replace waylandsink with autovideosink
```
gst-launch-1.0 -e --gst-debug=2 filesrc location=/opt/data/Animals_000_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<1920,1080>" ! queue ! autovideosink \
split. ! queue ! qtimlvconverter ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/opt/data/mobilenet_v2-mobilenet-v2-float.tflite ! queue ! qtimlpostprocess results=1 module=mobilenet labels=/opt/data/mobilenet_old.json settings="{\"confidence\": 51.0}" ! video/x-raw,format=BGRA,width=640,height=360 ! queue ! mixer.
```

#### Result
```
WARNING: erroneous pipeline: no element "qtimlpostprocess"
```

### Removed mlpostprocess and engine=fcv

```
gst-launch-1.0 -e --gst-debug=2 filesrc location=/home/ubuntu/inbox/testVideos/Draw_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<368,64>" ! queue ! waylandsink sync=false fullscreen=true \
split. ! queue ! qtimlvconverter engine=fcv ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=gpu;" model=/home/ubuntu/inbox/TFLite/mobilenet_v2-mobilenet-v2-float.tflite ! queue ! qtimlvclassification threshold=60.0 results=3 module=mobilenet labels=/home/ubuntu/inbox/TFLite/mobilenet.labels  ! video/x-raw,format=BGRA,width=368,height=64 ! queue ! mixer.
```


##### Error
```
0:00:01.325368014  6334 0xffff90001890 ERROR       ml-tflite-engine ml-tflite-engine-c-api.cc:806:gst_ml_tflite_engine_execute: Model execution failed!
0:00:01.327592150  6334 0xffff90000da0 ERROR   video-converter-engine gles-video-converter.cc:696:gst_gles_video_converter_new: Failed to open IB2C library, error: /lib/aarch64-linux-gnu/libIB2C.so.1: undefined symbol: glEGLImageTargetTexture2DOES!
0:00:01.327639750  6334 0xffff90000da0 ERROR   video-converter-engine video-converter-engine.c:200:gst_video_converter_engine_new: Failed to create backend converter!

````