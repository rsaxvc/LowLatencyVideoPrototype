#include <stdint.h>
#include "packet_server.h"

void packet_server::broadcast( const uint8_t*data, size_t bytes)
{
for (std::list<data_source*>::iterator iterator = targets.begin(), end = targets.end(); iterator != end; ++iterator)
	{
	(*iterator)->write( data, bytes );
	}
}

void packet_server::register_callback( data_source * source )
{
targets.push_back( source );
}

size_t packet_server::num_targets( void )
{
targets.size();
}
