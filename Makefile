all: capture x264_encode avcodec libavcexample
capture: capture.cpp
	g++ capture.cpp -o capture

x264_encode: x264_encode.c
	gcc x264_encode.c -o x264_encode -lx264 -g

avcodec: avcodec_sample.0.5.0.c destreamer.h
	gcc avcodec_sample.0.5.0.c -o avcodec -Wall -lswscale -lavcodec

libavcexample: libavcexample.c
	gcc libavcexample.c -o libavcexample -Wall -lavcodec -lavformat
