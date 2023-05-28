#ifndef BROWNIANBOT_SOCKET_UTILS_H
#define BROWNIANBOT_SOCKET_UTILS_H

#include "../error/error.h"

int connect_to_unix_socket(const char* socket_name, Error** error);

void sendall(int socket_fd, const char* buffer, int count, Error** error);


#endif
