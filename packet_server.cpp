#include <stdint.h>
#include "packet_server.h"

void packet_server::broadcast( const uint8_t*data, size_t bytes)
{
for (std::list<target>::const_iterator iterator = targets.begin(), end = targets.end(); iterator != end; ++iterator)
	{
	iterator->func( iterator->data, data, bytes );
	}
}

void packet_server::register_callback( packet_server_callback func, void * data )
{
target t;
t.func = func;
t.data = data;
targets.push_back( t );
}

size_t packet_server::num_targets( void )
{
targets.size();
}
