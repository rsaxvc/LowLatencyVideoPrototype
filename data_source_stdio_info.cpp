#include <cstdio>
#include "data_source_stdio_info.h"

void data_source_stdio_info::write( const uint8_t * data, size_t bytes )
{
fprintf(stdout,"packet %i: %i bytes\n", ++num_packets, bytes);
}
