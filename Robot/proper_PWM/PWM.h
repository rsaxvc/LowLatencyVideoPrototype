#ifndef PWM_H
#define PWM_H

#include <stdint.h>

class PWM_driver
	{
	public:
		PWM_driver(const char * path_with_trailing_slash, int8_t default_value );
		~PWM_driver();
		void set_level( int8_t level );

	private:
		int write_int( int fd, int8_t value );
		int run_fd;
		int freq_fd;
		int duty_fd;
		int8_t default_value;
		char * path;
	};

#endif
