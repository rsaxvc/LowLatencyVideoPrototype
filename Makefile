FLAGS += `pkg-config --cflags --libs x264`
FLAGS += `pkg-config --cflags --libs libavutil`
FLAGS += `pkg-config --cflags --libs libavformat`
FLAGS += `pkg-config --cflags --libs libavcodec`
FLAGS += `pkg-config --cflags --libs libswscale`
FLAGS += `pkg-config --cflags --libs libv4l2`

all: capture x264_encode avcodec libavcexample v4l2_enumerate
capture: capture.cpp config.h
	g++ $< -o $@ $(FLAGS)

x264_encode: x264_encode.c config.h
	gcc $< -o $@ $(FLAGS)

avcodec: avcodec_sample.0.5.0.c destreamer.h config.h
	gcc $< -o $@ $(FLAGS)

libavcexample: libavcexample.c config.h
	gcc $< -o $@ $(FLAGS)

v4l2_enumerate: v4l2_enumerate.cpp
	g++ $< -o $@ $(FLAGS)

clean:
	rm -f capture x264_encode avcodec libavcexample v4l2_enumerate
