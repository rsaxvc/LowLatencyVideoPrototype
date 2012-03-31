#include <limits.h>

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "PWM.h"

#define PWM_FREQ 50

char * sprintf_alloc( const char * format, ... ) __attribute__ ((format (printf, 1, 2)));

char * sprintf_alloc( const char * format, ... )
{
char * buffer;
va_list ap;
int len;

//get size
va_start(ap, format);
len = vsnprintf(NULL, 0, format, ap);
va_end(ap);

//get buffer
buffer = (char*)malloc( len + 1 );

va_start(ap, format);
vsnprintf(buffer, len + 1, format, ap );
va_end(ap);

return buffer;
}

PWM_driver::PWM_driver( const char * path_with_slash, int8_t default_v )
{
char * buffer;

path = strdup( path_with_slash );

buffer = sprintf_alloc("cat %srequest", path );
printf("Running:%s\n", buffer);
//system(buffer);
free( buffer );

buffer = sprintf_alloc("echo %i > %speriod_freq", PWM_FREQ, path );
printf("Running:%s\n", buffer);
//system(buffer);
free( buffer );

set_level( default_value );
}

PWM_driver::~PWM_driver()
{
char * buffer;
set_level( default_value );

buffer = sprintf_alloc("echo 0 > %srequest",path);
printf("Running%s\n",buffer);
//system(buffer);
free( buffer );
}

void PWM_driver::set_level( int8_t level )
{
char * buffer;
assert( level <= 100 );
assert( level >= 0 );

buffer = sprintf_alloc("echo %i > %sduty_percent", level, path);
printf("Running:%s\n",buffer);
//system(buffer);
free( buffer );

buffer = sprintf_alloc("echo 1 > %srun",path);
printf("Running:%s\n",buffer);
//system(buffer);
free( buffer );

}
