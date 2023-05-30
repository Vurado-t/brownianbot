#ifndef BROWNIANBOT_SOCKET_UTILS_H
#define BROWNIANBOT_SOCKET_UTILS_H


#include "../error/error.h"

typedef struct sockaddr_un SocketUnixAddress;

void init_socket_unix_address(const char* socket_name, SocketUnixAddress* unix_address);

int get_async_socket_factory(const char* socket_name, int backlog_length, Error** error);

int connect_to_socket(const char* socket_name, Error** error);

void send_string(int socket_fd, const char* str, Error** error);

char* recv_line(int socket_fd, char* buffer, int char_count, Error** error);


#endif
