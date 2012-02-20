#include <cstring>

#include "config.h"

#include "data_source_udp.h"
#include "data_source_stdio_info.h"
#include "packet_server.h"

#define my_strlen( _ptr )  strlen( (const char*)_ptr )

int main()
{
static uint8_t test1[]="Message:  1\n";
static uint8_t test2[]="Message: 2 \n";
static uint8_t test3[]="Message:3  \n";

packet_server server;

data_source_stdio_info stdio_info;
server.register_callback(&stdio_info);

data_source_udp udp_src("localhost",UDP_PORT_NUMBER);
server.register_callback(&udp_src);

server.broadcast(test1,my_strlen(test1));
server.broadcast(test2,my_strlen(test2));
server.broadcast(test3,my_strlen(test3));

}
