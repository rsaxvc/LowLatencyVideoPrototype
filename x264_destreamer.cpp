using namespace std;

#include "x264_destreamer.h"

void x264_destreamer::write( const uint8_t * data, size_t bytes )
{
while( bytes )
	{
	input( *data );
	bytes--;
	data++;
	}
}

void x264_destreamer::input( uint8_t byte )
{
previous_state = ( ( previous_state & 0x00FFFFFF ) << 8 ) | byte;

if( previous_state == 0x00000001 )
	{
	if( sync )
		{
		server.broadcast( &buffer[0], buffer.size() );
		buffer.clear();
		}
	sync = true;
	}

if( sync )
	{
	buffer.push_back( ( previous_state & 0xFF000000 ) >> 24 );
	}

}

x264_destreamer::x264_destreamer()
{
previous_state = 0xFFFFFFFF;
sync = false;
}

x264_destreamer::~x264_destreamer()
{
}
