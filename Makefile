FLAGS += `pkg-config --cflags --libs x264`
FLAGS += `pkg-config --cflags --libs libavutil`
FLAGS += `pkg-config --cflags --libs libavformat`
FLAGS += `pkg-config --cflags --libs libavcodec`
FLAGS += `pkg-config --cflags --libs libswscale`
FLAGS += `pkg-config --cflags --libs libv4l2`
FLAGS += `pkg-config --cflags --libs opencv`
FLAGS += -g

all: .depend\
	capture\
	x264_encode\
	avcodec\
	avcodec_rsa\
	libavcexample\
	v4l2_enumerate\
	test_data_source\
	test_data_source_ocv

SOURCES=`ls *.cpp *.c`

.depend:
	fastdep $(SOURCES) > .depend

-include .depend

capture: capture.o
	g++ $? -o $@ $(FLAGS)

x264_encode: x264_encode.o
	gcc $? -o $@ $(FLAGS)

avcodec: avcodec_sample.0.5.0.o 
	gcc $? -o $@ $(FLAGS)

avcodec_rsa: avcodec_rsa.o
	gcc $? -o $@ $(FLAGS)

libavcexample: libavcexample.o
	gcc $? -o $@ $(FLAGS)

v4l2_enumerate: v4l2_enumerate.o
	g++ $? -o $@ $(FLAGS)

test_data_source: test_data_source.o packet_server.o data_source_stdio.o data_source_stdio_info.o
	g++ $? -o $@ $(FLAGS)

test_data_source_ocv: test_data_source_ocv.o packet_server.o data_source_stdio_info.o data_source_ocv_avcodec.o
	g++ $? -o $@ $(FLAGS)

clean:
	rm -f capture x264_encode avcodec libavcexample v4l2_enumerate test_data_source .depend *.o avcodec_rsa test_data_source_ocv
