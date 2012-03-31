#include <cstdio>
#include "signal_control.h"

sigset_t ALL_SIGNALS;

class side_affector
	{
	public:
	side_affector();
	~side_affector(){};
	};

side_affector::side_affector()
{
sigfillset( &ALL_SIGNALS );
printf("Signals up\n");
}

side_affector init_the_signals;
