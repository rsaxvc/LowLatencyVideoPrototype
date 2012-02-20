#include <cstdio>
#include "data_source_stdio_info.h"

data_source_stdio_info::data_source_stdio_info()
{
num_packets=0;
}

data_source_stdio_info::~data_source_stdio_info()
{
}

void data_source_stdio_info::write( const uint8_t * data, size_t bytes )
{
fprintf(stdout,"Received packet %i: %i bytes\n", ++num_packets, bytes);
}
