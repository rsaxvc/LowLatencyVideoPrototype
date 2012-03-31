#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL_net.h>

#include "PWM.h"
#include "signal_control.h"
#include "RCPacket.h"

#define STEERING_DEVICE ""
#define THROTTLE_DEVICE "/sys/class/pwm/ecap.0/"

PWM_driver throttle( THROTTLE_DEVICE, 0 );
PWM_driver steering( STEERING_DEVICE, 0 );

void estop( void )
{
/*kill motor*/
throttle.set_level(0);

/*center steering*/
steering.set_level(0);
}


void catch_sigint(int sig)
{
estop();

/*bail*/
exit(1);
}

int main(int argc, char **argv)
{
	SDLNet_SocketSet set;
	UDPsocket udpsock;       /* Socket descriptor */
	UDPpacket *p;       /* Pointer to packet memory */
	int quit;

	signal(SIGINT, catch_sigint );

	if (SDLNet_Init() < 0)/* Initialize SDL_net */
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	if (!(udpsock = SDLNet_UDP_Open(2000)))/* Open a socket */
	{
		fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	set=SDLNet_AllocSocketSet(1);
	if(!set)
	{
		printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE); //most of the time this is a major error, but do what you want.
	}

	if( -1 == SDLNet_UDP_AddSocket(set,udpsock) )
	{
	    printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	if (!(p = SDLNet_AllocPacket( sizeof( remote_control_packet ) ) ) )/* Make space for the packet */
	{
		fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	/* Main loop */
	quit = 0;
	while (!quit)
	{
		/* Wait a packet. UDP_Recv returns != 0 if a packet is coming */
		DISABLE_SIGNALS();
		if (
			1 == SDLNet_CheckSockets(set, 1000) &&
			1 == SDLNet_UDP_Recv( udpsock, p ) &&
			p->len == sizeof( remote_control_packet ) )
		{
			int8_t steer_val = ((remote_control_packet*)p->data)->steering;
			int8_t thrtl_val = ((remote_control_packet*)p->data)->throttle;

			printf("Packet S:%04i T:%04i\n", (int)steer_val, (int)thrtl_val );
			steering.set_level( steer_val );
			throttle.set_level( thrtl_val );
		}
		else
		{
			printf("no update from remote, halting\n");
			estop();
		}
		ENABLE_SIGNALS();
	}

	/* Clean and exit */
	SDLNet_FreePacket(p);
	SDLNet_Quit();
	return EXIT_SUCCESS;
}
