#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "PWM.h"

#define PWM_FREQ 50

int PWM_driver::write_int( int fd, int8_t value )
{
char buffer[100];
snprintf( buffer, sizeof( buffer ), "%i\n", value );
return write( fd, buffer, strlen( buffer ) + 1 );
}

PWM_driver::PWM_driver( const char * path_with_slash, int8_t default_v )
{
char buffer[1024];

path = strdup( path_with_slash );

sprintf( buffer, "cat %s%s", path, "request");
printf("running: %s", buffer );
system( buffer );

sprintf( buffer, "%s%s", path, "run");
run_fd = open( buffer, O_WRONLY | O_DSYNC );
if( run_fd == -1 )
	{
	printf("UNABLE TO OPEN:%s\n",buffer);
	exit(1);
	}

sprintf( buffer, "%s%s", path, "period_freq");
freq_fd = open( buffer, O_WRONLY | O_DSYNC );
if( freq_fd == -1 )
	{
	printf("UNABLE TO OPEN:%s\n",buffer);
	exit(1);
	}

sprintf( buffer, "%s%s", path, "duty_percent");
duty_fd = open( buffer, O_WRONLY );
if( duty_fd == -1 )
	{
	printf("UNABLE TO OPEN:%s\n",buffer);
	exit(1);
	}

write_int( freq_fd, PWM_FREQ );
default_value = default_v;
set_level( default_value );
}

PWM_driver::~PWM_driver()
{
set_level( default_value );
close( run_fd );run_fd=-1;
close( duty_fd );duty_fd=-1;
close( freq_fd );freq_fd=-1;

char buffer[1024];

sprintf( buffer, "echo 0 > %s%s", path, "request");
printf("running: %s", buffer );
system( buffer );

free(path);
}

void PWM_driver::set_level( int8_t level )
{
assert( level <= 100 );
assert( level >= 0 );

write_int( duty_fd, level );
write_int( run_fd, 1 );
}
