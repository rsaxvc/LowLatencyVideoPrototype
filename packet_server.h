#ifndef PACKET_SERVER_H
#define PACKET_SERVER_H

#include <list>
#include <stdint.h>

#include "data_source.h"

//broadcast calls each registered callback with data/bytes
class packet_server
	{
	public:
	void broadcast( const uint8_t * data, size_t bytes);
	void register_callback( data_source * target );
	size_t num_targets();

	private:

	std::list<data_source*> targets;
	};

#endif
