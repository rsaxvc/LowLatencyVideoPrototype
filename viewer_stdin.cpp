#include <iostream>
#include <cstdio>

#include "data_source_ocv_avcodec.h"
#include "data_source_stdio_info.h"
#include "x264_destreamer.h"

using namespace std;

int main(int numArgs, const char * args[] )
{
x264_destreamer ds;
data_source_ocv_avcodec oavc("output");
data_source_stdio_info info;

ds.server.register_callback( &oavc );
ds.server.register_callback( &info );

while( !feof( stdin) )
	{
	uint8_t data;
	data = getchar();
	ds.write( &data, sizeof( data ) );
	}
}
