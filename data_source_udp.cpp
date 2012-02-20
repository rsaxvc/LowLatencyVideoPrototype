//RSA: bits taken from http://www.gamedev.net/topic/310343-udp-client-server-echo-example/
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"

#include <iostream>
#include "data_source_udp.h"

data_source_udp::data_source_udp( const char * hostname, int portno )
{
sd = -1;
int rc, i, n, echoLen, error;
struct sockaddr_in cliAddr, echoServAddr;
struct hostent *h;

/* get server IP address */
h = gethostbyname(hostname);
if(h==NULL)
	{
	printf("UDP: unknown host '%s' \n", hostname);
	exit(1);
	}

printf("UDP: sending data to '%s' (IP : %s) \n", h->h_name, inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));

remoteServAddr.sin_family = h->h_addrtype;
memcpy((char *) &remoteServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
remoteServAddr.sin_port = htons(UDP_PORT_NUMBER);

/* socket creation */
sd = socket(AF_INET,SOCK_DGRAM,0);
if(sd<0)
	{
	printf("UDP: cannot open socket \n");
	}

/* bind any port */
cliAddr.sin_family = AF_INET;
cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
cliAddr.sin_port = htons(0);

rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
if(rc<0)
	{
	printf("UDP: cannot bind port\n");
	close(sd);
	sd=-1;
	}

}

data_source_udp::~data_source_udp()
{
if( sd >= 0 )
	{
	close( sd );
	}
}

void data_source_udp::write( const uint8_t * data, size_t bytes )
{
if( sendto(sd, data, bytes, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr)) < 0 )
	{
	printf("UDP: could not send data\n");
	close(sd);
	sd=-1;
	}
}

