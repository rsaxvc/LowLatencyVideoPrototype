#!/bin/sh
gst-launch -vvv \
udpsrc port=10000 \
! mpegtsdemux \
! ffdec_h264 \
! ffmpegcolorspace \
! videoscale method=0 add-borders=true \
! ximagesink sync=false
