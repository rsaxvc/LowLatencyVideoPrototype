#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "data_source_tcp_server.h"

data_source_tcp_server::data_source_tcp_server( int portno )
{
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd >= 0)
	{
	int flag = 1;
	if( setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,(char *) &flag,sizeof(int) )< 0 )
		{
		printf("Unable to set TCP_NODELAY\n");
		}

	memset((char *) &serv_addr, 0x00, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)
		{
		listen(sockfd,1);
		clilen = sizeof(cli_addr);
		printf("Listening on port %i\n",portno);
		fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (fd >= 0)
			{
			printf("Server connected port %i\n",portno);
			}
		else
			{
			printf("ERROR on accept\n");
			}
		}
	else
		{
		printf("ERROR on binding\n");
		}
	}
else
	{
	printf("ERROR opening socket\n");
	}
}

data_source_tcp_server::~data_source_tcp_server()
{
if( fd != -1 )
	{
	close( fd );
	}

if( sockfd != -1 )
	{
	close(sockfd);
	}
}

void data_source_tcp_server::write( const uint8_t * data, size_t bytes )
{
::write( fd, data, bytes );
}
