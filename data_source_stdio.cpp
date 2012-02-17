#include <cstdio>
#include "data_source_stdio.h"

void data_source_stdio::write( void * data_source_ptr, const uint8_t * data, size_t bytes )
{
size_t i;
for( i = 0; i < bytes; ++i )
	{
	fprintf(stdout,"%c",data[i]);
	}
fprintf(stdout,"\n");
}
