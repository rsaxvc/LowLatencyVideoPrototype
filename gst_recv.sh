#!/bin/sh
gst-launch -vvv \
udpsrc port=10000 \
! mpegtsdemux \
! ffdec_h264 \
! ffmpegcolorspace \
! clockoverlay auto-resize=false shaded-background=true xpad=0 ypad=0 deltay=31  \
! videoscale method=0 add-borders=true \
! ximagesink sync=false
