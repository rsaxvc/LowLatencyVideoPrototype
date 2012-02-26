#ifndef DATA_SOURCE_TCP_SERVER_H
#define DATA_SOURCE_TCP_SERVER_H

#include <stddef.h>
#include <stdint.h>
#include "data_source.h"

//creates a TCP server, blocks until connect, then forwards data
class data_source_tcp_server: public data_source
	{
	public:
	data_source_tcp_server(int portno);
	~data_source_tcp_server();
	void write( const uint8_t * data, size_t bytes );

	private:
	int sockfd;
	int fd;
	};

#endif
