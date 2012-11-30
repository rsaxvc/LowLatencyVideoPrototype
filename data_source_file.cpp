#include "data_source_file.h"

#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

data_source_file::data_source_file( const char * fname )
{
fd = open( fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP );
if( fd < 0 )
	{
	std::cout<<"Couldn't open "<<fname<<std::endl;
	}
}

data_source_file::~data_source_file()
{
if( fd != -1 )
	{
	close( fd );
	}
}

void data_source_file::write( const uint8_t * data, size_t bytes )
{
::write( fd, data, bytes );
}
