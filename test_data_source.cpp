#include "data_source.h"
#include "data_source_stdio.h"
#include "data_source_stdio_info.h"
#include "packet_server.h"

int main()
{
static uint8_t nothing[]="Should not print";
static uint8_t test1[]="Should print once";
static uint8_t test2[]="Should print once, with the length after";

packet_server server;
server.broadcast(nothing,sizeof(nothing));

data_source_stdio stdio_src;
server.register_callback(stdio_src.write,&stdio_src);
server.broadcast(test1,sizeof(test1));

data_source_stdio_info stdio_info;
server.register_callback(stdio_info.write,&stdio_info);
server.broadcast(test2,sizeof(test2));
}
