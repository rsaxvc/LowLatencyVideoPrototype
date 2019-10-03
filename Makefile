# pkg-config packages list
PKGS := x264 libavutil libavformat libavcodec libswscale libv4l2 opencv sdl2 SDL_net
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LDFLAGS := $(shell pkg-config --libs $(PKGS))

ADD_CFLAGS := -g -D__STDC_CONSTANT_MACROS
ADD_LDFLAGS := -lrt

CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(CFLAGS)
LDFLAGS := $(PKG_LDFLAGS) $(ADD_LDFLAGS) $(LDFLAGS)
CXXFLAGS := $(CFLAGS)

ALL_BUILDS = \
	encoder\
	encoder_udp\
	v4l2_enumerate\
	test_data_source\
	test_data_source_tcp_server\
	test_data_source_udp\
	test_data_source_ocv\
	viewer_stdin\
	viewer_sdl\

all: .depend $(ALL_BUILDS)

SOURCES=`ls *.cpp`

.depend:
	fastdep $(SOURCES) > .depend

-include .depend

encoder: encoder.o
	g++ $? $(CFLAGS) -o $@ $(LDFLAGS)

encoder_udp: encoder_udp.o
	g++ $? -o $@ $(LDFLAGS)

v4l2_enumerate: v4l2_enumerate.o
	g++ $? -o $@ $(LDFLAGS)

viewer_stdin: viewer_stdin.o data_source_ocv_avcodec.o x264_destreamer.o packet_server.o data_source_stdio_info.o
	g++ $? -o $@ $(LDFLAGS)

viewer_sdl: viewer_sdl.o x264_destreamer.o packet_server.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source: test_data_source.o packet_server.o data_source_stdio.o data_source_stdio_info.o data_source_file.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source_tcp_server: test_data_source_tcp_server.o packet_server.o data_source_stdio_info.o data_source_tcp_server.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source_udp: test_data_source_udp.o packet_server.o data_source_stdio_info.o data_source_udp.o data_source_stdio.o
	g++ $? -o $@ $(LDFLAGS)

test_data_source_ocv: test_data_source_ocv.o packet_server.o data_source_stdio_info.o data_source_ocv_avcodec.o
	g++ $? -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(ALL_BUILDS)
