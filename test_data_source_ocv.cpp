#include <iostream>
#include <cstdlib>
#include "data_source_stdio_info.h"
#include "data_source_ocv_avcodec.h"
#include "packet_server.h"
#include "destreamer.h"

int main(int num_args, const char * const args[] )
{
if( num_args !=2 )
	{
	std::cout<<"usage:"<<args[0]<<" input_file"<<std::endl;
	exit(4);
	}

open_264(args[1]);

packet_server server;

static uint8_t test2[]="Should print once, with the length after";

data_source_ocv_avcodec viewer("output");
server.register_callback(&viewer);

data_source_stdio_info stdio_info;
server.register_callback(&stdio_info);

while( get_next_block() )
	{
	server.broadcast(&ds.buffer[ds.last_pos], ds.cur_pos-ds.last_pos);
	}

close_264();
}
