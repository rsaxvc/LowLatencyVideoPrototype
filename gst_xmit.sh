#!/bin/sh
#v4l2src device=/dev/video0 always-copy=false
gst-launch -vvv \
videotestsrc is-live=true \
! video/x-raw-yuv,width=320,height=240,framerate=30/1 \
! videoscale method=0 \
! video/x-raw-yuv,width=160,height=120 \
! clockoverlay auto-resize=false shaded-background=true xpad=0 ypad=0 deltay=0 \
! x264enc \
tune=zerolatency \
intra-refresh=true \
profile=high \
byte-stream=true \
bitrate=220 \
sliced-threads=true \
speed-preset=2 \
! mpegtsmux \
! udpsink port=10000 host=127.0.0.1 ts-offset=0 sync=false
