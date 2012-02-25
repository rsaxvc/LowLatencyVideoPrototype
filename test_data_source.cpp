#include <cstring>

#include "data_source.h"
#include "data_source_file.h"
#include "data_source_stdio.h"
#include "data_source_stdio_info.h"
#include "packet_server.h"

#define my_strlen( _ptr )  strlen( (const char*)_ptr )

int main()
{
static uint8_t nothing[]="Should not print";
static uint8_t test1[]="Ninja!";
static uint8_t test2[]="Should have printed 3 Ninja!s, ";
static uint8_t test3[]="But the file should only have one, and this one has stats";

packet_server server;
server.broadcast(nothing,my_strlen(nothing));

data_source_stdio stdio_src;
server.register_callback(&stdio_src);
server.broadcast(test1,my_strlen(test1));
server.broadcast(test1,my_strlen(test1));

data_source_file file_src("test_output.txt");
server.register_callback(&file_src);
server.broadcast(test1,my_strlen(test1));

data_source_stdio_info stdio_info;
server.register_callback(&stdio_info);
server.broadcast(test2,my_strlen(test2));

server.broadcast(test3,my_strlen(test3));

}
