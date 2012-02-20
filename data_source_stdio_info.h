#ifndef DATA_SOURCE_STDIO_INFO_H
#define DATA_SOURCE_STDIO_INFO_H

#include <stddef.h>
#include <stdint.h>

#include "data_source.h"

class data_source_stdio_info: public data_source
	{
	public:
	data_source_stdio_info();
	~data_source_stdio_info();
	void write( const uint8_t * data, size_t bytes );

	private:
	int num_packets;
	};

#endif
