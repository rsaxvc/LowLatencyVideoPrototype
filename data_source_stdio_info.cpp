#include <cstdio>
#include "data_source_stdio_info.h"

void data_source_stdio_info::write( const uint8_t * data, size_t bytes )
{
fprintf(stdout,"Received packet: %i bytes\n", bytes);
}
