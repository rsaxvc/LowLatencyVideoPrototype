#include <cstdio>
#include "data_source_stdio.h"

void data_source_stdio::write( const uint8_t * data, size_t bytes )
{
fwrite(data,1,bytes,stdout);
}
