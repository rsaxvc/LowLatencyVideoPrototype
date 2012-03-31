COMPILE_FLAGS = -g -O0
COMPILE_FLAGS += `sdl-config --cflags`

CFLAGS += $(COMPILE_FLAGS)
CPPFLAGS += $(COMPILE_FLAGS)

LDFLAGS += `sdl-config --libs`
LDFLAGS +=  -lSDL_net 

ALL_BUILDS = \
	motor_controller

all: .depend $(ALL_BUILDS)

SOURCES=`ls *.cpp`

.depend:
	fastdep $(SOURCES) > .depend

-include .depend

motor_controller: mc.o PWM.o signal_control.o
	g++ $? -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(ALL_BUILDS) .depend
