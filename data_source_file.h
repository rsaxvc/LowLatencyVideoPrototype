#ifndef DATA_SOURCE_FILE_H
#define DATA_SOURCE_FILE_H

#include <stddef.h>
#include <stdint.h>
#include "data_source.h"

//writes output to a file
class data_source_file: public data_source
	{
	public:
	data_source_file(const char * fname);
	~data_source_file();
	void write( const uint8_t * data, size_t bytes );
	private:
	int fd;
	};

#endif
