#ifndef PACKET_SERVER_H
#define PACKET_SERVER_H

#include <list>
#include <stdint.h>

typedef void (*packet_server_callback)(void*userdata,const uint8_t*data,size_t bytes);

class packet_server
	{
	void broadcast( const uint8_t*data, size_t bytes);
	void register_callback( packet_server_callback, void * userdata );
	size_t num_targets();

	private:
	struct target
		{
		packet_server_callback func;
		void *                 data;
		};


	std::list<target> targets;
	};

#endif
