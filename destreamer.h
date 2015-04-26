#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct
	{
	unsigned char * buffer;
	int size;

	int last_pos;
	int cur_pos;
	}destreamer;

static destreamer ds;
static void open_264( const char * fname )
{
struct stat statbuf;
int fd;
if( stat(fname, &statbuf) < 0 )
	{
	printf("Failed to open %s\n",fname);
	return;
	}
fd = open( fname, O_RDONLY );
ds.buffer = (uint8_t*)malloc( statbuf.st_size );
read( fd, ds.buffer, statbuf.st_size );
ds.size = statbuf.st_size;
printf("opened %s, size=%i\n",fname,ds.size);
close( fd );
ds.cur_pos=0;
ds.last_pos=0;
}

static int get_next_block( void )
{
ds.last_pos = ds.cur_pos;
ds.cur_pos+=1;
for( ; ds.cur_pos < ds.size - 3; ++ds.cur_pos )
	{
	//printf("cur_pos:%i, buffer:%08x\n",ds.cur_pos,ds.buffer);
	if( ds.buffer[ds.cur_pos+0]==0 &&
	    ds.buffer[ds.cur_pos+1]==0 &&
	    ds.buffer[ds.cur_pos+2]==0 &&
	    ds.buffer[ds.cur_pos+3]==1 )
		{
		return 1;
		}
	}
printf("end of stream\n");
return 0;
}

static void close_264( void )
{
free( ds.buffer );
ds.buffer = NULL;
}
