#ifndef X264_DESTREAMER_H
#define X264_DESTREAMER_H

#include <vector>
#include <stdint.h>

#include "packet_server.h"

class x264_destreamer
	{
	public:
	x264_destreamer();
	~x264_destreamer();
	void write( const uint8_t * data, size_t bytes);
	packet_server server;

	private:
	void input( uint8_t byte );
	std::vector<uint8_t>buffer;
	uint32_t previous_state;
	bool sync;
	};

#endif
