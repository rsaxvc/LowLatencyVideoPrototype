#!/bin/sh
gst-launch -vvv \
v4l2src device=/dev/video0 always-copy=false \
! video/x-raw-yuv,width=320,height=240,framerate=30/1 \
! videoscale method=0 \
! video/x-raw-yuv,width=160,height=120 \
! x264enc \
tune=zerolatency \
intra-refresh=true \
profile=high \
byte-stream=true \
bitrate=220 \
! mpegtsmux \
! udpsink port=10000 host=127.0.0.1 ts-offset=0 sync=false
