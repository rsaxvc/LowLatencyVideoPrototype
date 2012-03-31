#ifndef RCPACKET_H
#define RCPACKET_H

#include <stdint.h>

typedef struct
	{
	int8_t steering;
	int8_t throttle;
	}remote_control_packet;

#endif
