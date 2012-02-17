FLAGS += `pkg-config --cflags --libs x264`
FLAGS += `pkg-config --cflags --libs libavutil`
FLAGS += `pkg-config --cflags --libs libavformat`
FLAGS += `pkg-config --cflags --libs libavcodec`
FLAGS += `pkg-config --cflags --libs libswscale`
FLAGS += `pkg-config --cflags --libs libv4l2`

all: capture x264_encode avcodec libavcexample v4l2_enumerate
capture: capture.cpp config.h
	g++ capture.cpp -o capture

x264_encode: x264_encode.c config.h
	gcc x264_encode.c -o x264_encode $(FLAGS) -g

avcodec: avcodec_sample.0.5.0.c destreamer.h config.h
	gcc avcodec_sample.0.5.0.c -o avcodec -Wall $(FLAGS)

libavcexample: libavcexample.c config.h
	gcc libavcexample.c -o libavcexample -Wall $(FLAGS)

v4l2_enumerate: v4l2_enumerate.cpp
	g++ v4l2_enumerate.cpp -o v4l2_enumerate -Wall $(FLAGS)

clean:
	rm -f capture x264_encode avcodec libavcexample v4l2_enumerate
