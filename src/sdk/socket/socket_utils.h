#ifndef BROWNIANBOT_SOCKET_UTILS_H
#define BROWNIANBOT_SOCKET_UTILS_H


#include "../error/error.h"

int get_socket_factory(const char* socket_name, int backlog_length, Error** error);

int connect_to_unix_socket(const char* socket_name, Error** error);

void send_string(int socket_fd, const char* str, Error** error);

char* recv_line(int socket_fd, char* buffer, int char_count, Error** error);


#endif
