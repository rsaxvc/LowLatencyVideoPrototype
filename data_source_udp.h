#ifndef DATA_SOURCE_UDP_H
#define DATA_SOURCE_UDP_H

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include "data_source.h"

//sends a UDP packet per write (unless fragged)
class data_source_udp: public data_source
	{
	public:
	data_source_udp(const char * hostname, int portno);
	~data_source_udp();
	void write( const uint8_t * data, size_t bytes );
	private:
	int sd;
	struct sockaddr_in remoteServAddr;
	};

#endif
