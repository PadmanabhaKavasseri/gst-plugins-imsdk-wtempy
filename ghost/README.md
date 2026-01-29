Template-Python
TemPy

## Bitbake Push Commands
git push ssh://review-android.quicinc.com:29418/meta-qti-gst HEAD:refs/for/imsdk.lnx.2.0.0

## Source Push Commands
git push ssh://review-android.quicinc.com:29418/platform/vendor/qcom-opensource/gst-plugins-qti-oss HEAD:refs/for/le-gst.lnx.2.1

### Pull Python3 Shared Objects
scp pkavasse@las-colo24-n9-10-02:/local/mnt/workspace/pkavasseri/blds/au062/build-qcom-wayland/tmp-glibc/sysroots-components/armv8-2a/python3/usr/lib/libpython3.12.so /mnt/c/Users/pkavasse/Downloads/.
adb push libpython3.12.so /usr/lib/.

## Debug Commands
```
export GST_DEBUG_FILE=/root/qtitempy_debug.log
export GST_DEBUG=0,qtitempy:5

```

### Grey Scale Pipeline CMD
```
LD_PRELOAD=/usr/lib/libpython3.12.so gst-launch-1.0 videotestsrc ! video/x-raw,width=320,height=240 ! videoconvert ! qtitempy script=./opencv_processor.py function=gray_scale_filter ! video/x-raw,width=320,height=240 ! videoconvert ! waylandsink

LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 videotestsrc ! video/x-raw,width=320,height=240 ! videoconvert ! qtitempy script=./opencv_processor.py function=gray_scale_filter ! video/x-raw,width=320,height=240 ! videoconvert ! autovideosink

```


## Classification Pipeline
```
gst-launch-1.0 -e --gst-debug=2 filesrc location=/home/ubuntu/testVideos/Animals_000_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<1920,1080>" ! queue ! autovideosink \
split. ! queue ! qtimlvconverter ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite ! queue ! qtimlpostprocess results=1 module=mobilenet labels=/home/ubuntu/TFLite/mobilenet.labels settings="{\"confidence\": 51.0}" ! video/x-raw,format=BGRA,width=640,height=360 ! queue ! mixer.

```


## Classification Pipeline With qtiTemPy as preProcess
```
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 -e filesrc location=/home/ubuntu/testVideos/Animals_000_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<1920,1080>" ! queue ! autovideosink \
split. ! queue ! qtitempy script=./opencv_processor.py function=mobilenet_preprocess ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite ! queue ! qtimlpostprocess results=1 module=mobilenet labels=/home/ubuntu/TFLite/mobilenet.labels settings="{\"confidence\": 51.0}" ! video/x-raw,format=BGRA,width=640,height=360 ! queue ! mixer.

```


## Classification Pipeline With qtiTemPy as postProcess
```
--gst-debug=2,qtitempy:5
export GST_DEBUG_FILE=/root/qtitempy_debug.log
export GST_DEBUG=0,qtitempy:7

LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 -e filesrc location=/home/ubuntu/testVideos/Animals_000_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<540,220>"  sink_1::position="<0,0>" ! queue ! autovideosink \
split. ! queue ! qtimlvconverter ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite ! queue ! qtitempy script=./opencv_processor.py function=mobilenet_postprocess_top1 ! video/x-raw,format=BGRA,width=640,height=360 ! queue ! mixer.

```

## Metamux pipeline - Yolo Detection
```
gst-launch-1.0 -e --gst-debug=2 \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings="{\"confidence\": 70.0}" \
filesrc location=/home/ubuntu/testVideos/Draw_720p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.

```


## Metamux pipeline with qtieTemPy as postProcess tensor to text yolo detection
```

LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 -e  \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
qtitempy name=postproc script=./yolov5_processor.py function=yolov5_postprocess_text \
filesrc location=/home/ubuntu/testVideos/Draw_720p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.


```

## Debug pipeline to decode message structure from mlpostprocess into metamux
```


gst-launch-1.0 -e --gst-debug=2 \
    qtimlvconverter name=preproc \
    qtimltflite name=inference delegate=external \
        external-delegate-path=libQnnTFLiteDelegate.so \
        external-delegate-options="QNNExternalDelegate,backend_type=htp;" \
        model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
    qtimlpostprocess name=postproc results=5 module=yolov5 \
        labels=/home/ubuntu/TFLite/yolov8.json \
        settings="{\"confidence\": 70.0}" \
    filesrc location=/home/ubuntu/testVideos/Draw_720p_180s_30FPS.mp4 ! \
        qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! \
        video/x-raw,format=NV12 ! queue ! \
        preproc. preproc. ! queue ! \
        inference. inference. ! queue ! \
        postproc. postproc. ! \
        text/x-raw,format=utf8 ! \
        multifilesink location="postproc_%05d.txt"


Result:
{ (structure)"ObjectDetection\,\ bounding-boxes\=\(structure\)\<\ \"person\\\,\\\ id\\\=\\\(uint\\\)256\\\,\\\ confidence\\\=\\\(double\\\)88.352188110351562\\\,\\\ color\\\=\\\(uint\\\)16711935\\\,\\\ rectangle\\\=\\\(float\\\)\\\<\\\ 0.017667993903160095\\\,\\\ 0.14358751475811005\\\,\\\ 0.17667993903160095\\\,\\\ 0.86152499914169312\\\ \\\>\\\;\"\,\ \"person\\\,\\\ id\\\=\\\(uint\\\)257\\\,\\\ confidence\\\=\\\(double\\\)86.846183776855469\\\,\\\ color\\\=\\\(uint\\\)16711935\\\,\\\ rectangle\\\=\\\(float\\\)\\\<\\\ 0.30035591125488281\\\,\\\ 0.24679103493690491\\\,\\\ 0.20696794986724854\\\,\\\ 0.72691166400909424\\\ \\\>\\\;\"\ \>\,\ timestamp\=\(guint64\)1835168501\,\ sequence-index\=\(uint\)1\,\ sequence-num-entries\=\(uint\)1\;" }

```



## Multifilesink after qtitempy to debug output
```


LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 -e --gst-debug=2 \
    qtimlvconverter name=preproc \
    qtimltflite name=inference delegate=external \
        external-delegate-path=libQnnTFLiteDelegate.so \
        external-delegate-options="QNNExternalDelegate,backend_type=htp;" \
        model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
    qtitempy name=postproc script=../yolov5_processor.py function=yolov5_postprocess_text \
    filesrc location=/home/ubuntu/testVideos/Draw_720p_180s_30FPS.mp4 ! \
        qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! \
        video/x-raw,format=NV12 ! queue ! \
        preproc. preproc. ! queue ! \
        inference. inference. ! queue ! \
        postproc. postproc. ! \
        text/x-raw,format=utf8 ! \
        multifilesink location="postproc_%05d.txt"


Result:
{ (structure)"ObjectDetection\,\ bounding-boxes\=\(structure\)\<\ \"person\\\,\\\ id\\\=\\\(uint\\\)256\\\,\\\ confidence\\\=\\\(double\\\)88.349999999999994\\\,\\\ color\\\=\\\(uint\\\)16711935\\\,\\\ rectangle\\\=\\\(float\\\)\\\<\\\ 0.017667990177869797\\\,\\\ 0.14358751475811005\\\,\\\ 0.17667992413043976\\\,\\\ 0.86152499914169312\\\ \\\>\\\;\"\ \>\,\ timestamp\=\(guint64\)1835168501\,\ sequence-index\=\(uint\)1\,\ sequence-num-entries\=\(uint\)1\;" }

```

### CSI Camera pipelines

## CSI Cam to HDMI out

```
gst-launch-1.0 -e qtiqmmfsrc camera=0 ! video/x-raw,format=NV12,width=1280,height=720,framerate=30/1


```

## CSI Cam to object detection GHOST

```
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 -e  \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
qtitempy name=postproc script=./yolov5_processor.py function=yolov5_postprocess_text \
qtiqmmfsrc camera=0 ! video/x-raw,format=NV12 ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.


```

## CSI Cam to object detection IMDSK

```
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libpython3.12.so gst-launch-1.0 -e  \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings="{\"confidence\": 70.0}" \
qtiqmmfsrc camera=0 ! video/x-raw,format=NV12 ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.


```
