#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stddef.h>
#include <stdint.h>

class data_source
	{
	public:
	virtual void write( const uint8_t * data, size_t bytes )=0;
	};

#endif
