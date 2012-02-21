COMPILE_FLAGS = -g

CFLAGS += $(COMPILE_FLAGS)
CPPFLAGS += $(COMPILE_FLAGS)

LDFLAGS += `pkg-config --libs x264`
LDFLAGS += `pkg-config --libs libavutil`
LDFLAGS += `pkg-config --libs libavformat`
LDFLAGS += `pkg-config --libs libavcodec`
LDFLAGS += `pkg-config --libs libswscale`
LDFLAGS += `pkg-config --libs libv4l2`
LDFLAGS += `pkg-config --libs opencv`

ALL_BUILDS = \
	capture\
	x264_encode\
	avcodec\
	avcodec_rsa\
	encoder\
	v4l2_enumerate\
	test_data_source\
	test_data_source_tcp_server\
	test_data_source_udp\
	test_data_source_ocv

all: .depend $(ALL_BUILDS)

SOURCES=`ls *.cpp *.c`

.depend:
	fastdep $(SOURCES) > .depend

-include .depend

capture: capture.o
	g++ $? -o $@ $(LDFLAGS)

x264_encode: x264_encode.o
	gcc $? -o $@ $(LDFLAGS)

avcodec: avcodec_sample.0.5.0.o 
	gcc $? -o $@ $(LDFLAGS)

avcodec_rsa: avcodec_rsa.o
	gcc $? -o $@ $(LDFLAGS)

encoder: encoder.o
	gcc $? -o $@ $(LDFLAGS)

v4l2_enumerate: v4l2_enumerate.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source: test_data_source.o packet_server.o data_source_stdio.o data_source_stdio_info.o data_source_file.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source_tcp_server: test_data_source_tcp_server.o packet_server.o data_source_stdio_info.o data_source_tcp_server.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source_udp: test_data_source_udp.o packet_server.o data_source_stdio_info.o data_source_udp.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source_ocv: test_data_source_ocv.o packet_server.o data_source_stdio_info.o data_source_ocv_avcodec.o
	g++ $? -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(ALL_BUILDS)
