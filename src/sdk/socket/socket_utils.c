#include <sys/socket.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include "socket_utils.h"
#include "../log/log.h"

void init_socket_unix_address(const char* socket_name, SocketUnixAddress* unix_address) {
    unix_address->sun_family = AF_UNIX;
    strncpy(unix_address->sun_path, socket_name, sizeof(unix_address->sun_path));
    unix_address->sun_path[sizeof(unix_address->sun_path) - 1] = '\0';
}

int get_async_socket_factory(const char* socket_name, int backlog_length, Error** error) {
    *error = NULL;

    unlink(socket_name);

    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        *error = get_error_from_errno("get_async_socket_factory socket creating");
        return -1;
    }

    struct sockaddr_un addr;
    init_socket_unix_address(socket_name, &addr);

    bool is_bind_ok = bind(fd, (struct sockaddr*)&addr, sizeof(addr)) >= 0;
    if (!is_bind_ok)
        *error = get_error_from_errno("get_async_socket_factory bind");

    bool is_non_block_ok = fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) >= 0;
    if (!is_non_block_ok && *error == NULL)
        *error = get_error_from_errno("get_async_socket_factory non block");

    bool is_listen_ok = listen(fd, backlog_length) >= 0;
    if (!is_listen_ok && *error == NULL)
        *error = get_error_from_errno("get_async_socket_factory listen");

    if (!is_bind_ok || !is_non_block_ok || !is_listen_ok) {
        if (close(fd) < 0)
            *error = attach_error(get_error_from_errno("get_async_socket_factory close"), *error);

        return -1;
    }

    return fd;
}

int connect_to_socket(const char* socket_name, Error** error) {
    *error = NULL;

    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        *error = get_error_from_errno("connect_to_socket socket creating");
        return -1;
    }

    struct sockaddr_un addr;
    init_socket_unix_address(socket_name, &addr);

    bool is_connect_ok = connect(fd, (struct sockaddr*)&addr, sizeof(addr)) >= 0;

    if (!is_connect_ok) {
        *error = get_error_from_errno("connect_to_socket socket config");

        if (close(fd) < 0)
            *error = attach_error(get_error_from_errno("connect_to_socket close"), *error);

        return -1;
    }

    return fd;
}

void send_string(int socket_fd, const char* str, Error** error) {
    *error = NULL;

    int length = (int)strlen(str);
    int sent_count = 0;

    while (length - sent_count > 0) {
        int last_send_count = (int)send(socket_fd, str + sent_count, length - sent_count, 0);

        if (last_send_count == -1) {
            *error = get_error_from_errno("send_string");
            return;
        }

        sent_count += last_send_count;
    }
}

char* recv_line(int socket_fd, char* buffer, int buffer_length, Error** error) {
    *error = NULL;

    int recv_count = 0;
    while (recv_count < buffer_length - 1) {
        int last_recv_count = (int)recv(socket_fd, buffer + recv_count, 1, 0);
        if (last_recv_count == -1) {
            *error = get_error_from_errno("recv_line");
            return NULL;
        }

        recv_count += last_recv_count;
        if (buffer[recv_count - 1] == '\n')
            break;
    }

    if (buffer[recv_count - 1] != '\n') {
        *error = get_error_from_message("Too small buffer");
        return NULL;
    }

    buffer[recv_count - 1] = '\0';

    return buffer;
}
