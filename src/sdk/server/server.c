#include <malloc.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include "server.h"
#include "../log/log.h"
#include "../socket/socket_utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

/*
 * Returns connections->length if there is no inactive connections
 * */
ConnectionContext* get_free_cell(const ArrayConnectionContext* connections) {
    int i = 0;

    for (; i < connections->length; i++) {
        if (connections->items[i].state != DISABLED)
            continue;

        return &connections->items[i];
    }

    return NULL;
}

void reset_receiving_buffer(ConnectionContext* connection) {
    connection->free_receive_bytes_count = RECEIVE_BUFFER_LENGTH;
    connection->request_start_offset = 0;
    connection->request_current_offset = 0;
}

void start_receiving(ConnectionContext* connection) {
    reset_receiving_buffer(connection);
    connection->state = RECEIVING;
}

int try_accept_connection(ServerState* server_state) {
    Error* error = NULL;

    socklen_t socklen;

    // setting of __addr will give an error (Invalid arguments)
    int fd = accept(server_state->socket_factory_fd, NULL, &socklen);
    if (fd < 0) {
        error = get_error_from_errno("try_accept_connection accept");
        if (error->code != 11) // Not temporary unavailable
            log_error(error);

        free_error(error);
        return -1;
    }

    if (fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) < 0) {
        error = get_error_from_errno("try_accept_connection nonblock");
        log_error(error);
        free_error(error);
        close(fd);
        return -1;
    }

    ConnectionContext* connection = get_free_cell(&server_state->connections);
    connection->socket_fd = fd;

    start_receiving(connection);

    return fd;
}

void disable_connection(ConnectionContext* connection, Error** error) {
    *error = NULL;

    if (close(connection->socket_fd) < 0) {
        *error = get_error_from_errno("disable_connection");
        return;
    }

    connection->state = DISABLED;
}

void read_into_inner_buffer(ConnectionContext* connection) {
    int count = (int)recv(connection->socket_fd,
                          connection->receive_buffer + connection->request_current_offset,
                          connection->free_receive_bytes_count,
                          0);

    if (count <= 0)
        return;

    connection->request_current_offset = (connection->request_current_offset + count) % RECEIVE_BUFFER_LENGTH;
    connection->free_receive_bytes_count -= count;
}

long parse_long(ConnectionContext* connection, Error** error) {
    *error = NULL;

    long number = 0;
    int start_offset = connection->request_start_offset;
    int current_offset = connection->request_start_offset;
    bool is_negative = false;

    while (connection->receive_buffer[current_offset % RECEIVE_BUFFER_LENGTH] != '\n') {
        int length = current_offset - start_offset;

        if (length > 10) {
            *error = get_error_from_message("Too long request, no line break");
            break;
        }

        char current_char = connection->receive_buffer[current_offset % RECEIVE_BUFFER_LENGTH];

        if (current_char == '-' && !is_negative) {
            is_negative = true;
        }
        else if (isdigit(current_char)) {
            number = number * 10 + current_char - '0';
        }
        else {
            *error = get_error_from_message("Request may contain digits and one minus");
            break;
        }

        current_offset++;
    }

    if (*error != NULL) {
        reset_receiving_buffer(connection);
        return -1;
    }

    connection->request_start_offset = (current_offset + 1) % RECEIVE_BUFFER_LENGTH;
    connection->free_receive_bytes_count += current_offset - start_offset + 1;

    return is_negative ? -number : number;
}

void start_sending_response(ConnectionContext* connection, long response) {
    sprintf(connection->send_buffer, "%li\n", response);
    connection->sent_count = 0;
    connection->response_length = (int)strlen(connection->send_buffer);
    connection->state = SENDING;
}

bool try_read(ConnectionContext* connection) {
    if ((connection->poll_revents & POLLIN) == 0)
        return false;

    read_into_inner_buffer(connection);

    return true;
}

void handle_sending(ConnectionContext* connection) {
    int count = (int)send(connection->socket_fd, connection->send_buffer + connection->sent_count,
                     connection->response_length - connection->sent_count, 0);

    if (count <= 0)
        return;

    connection->sent_count += count;

    if (connection->sent_count == connection->response_length) {
        log_fmt_msg(INFO, "Send completed for fd %d ", connection->socket_fd);
        start_receiving(connection);
    }
}

void handle_receiving(ServerState* server_state, ConnectionContext* connection, Error** error) {
    *error = NULL;

    if (!try_read(connection))
        return;

    long request = parse_long(connection, error);
    if (*error != NULL)
        return;

    log_fmt_msg(INFO, "Received %d from fd(%d)", request, connection->socket_fd);

    server_state->counter += request;
    log_fmt_msg(INFO, "Updated server counter: %d", server_state->counter);

    start_sending_response(connection, server_state->counter);
    log_fmt_msg(INFO, "Started sending %d to fd %d", server_state->counter, connection->socket_fd);
}

void handle_poll_events(ServerState* server_state) {
    Error* error = NULL;

    for (int i = 0; i < server_state->connections.length; i++) {
        ConnectionContext* connection = &server_state->connections.items[i];

        if (connection->state == DISABLED)
            continue;

        if (connection->poll_revents & POLLERR) {
            log_fmt_msg(ERROR, "Socket error fd: '%d'", connection->socket_fd);
            disable_connection(connection, &error);
            if (error != NULL) {
                log_error(error);
                free_error(error);
            }

            continue;
        }

        if (connection->poll_revents & POLLHUP) {
            log_fmt_msg(INFO, "Disconnected fd '%d'", connection->socket_fd);
            disable_connection(connection, &error);
            if (error != NULL) {
                log_error(error);
                free_error(error);
            }

            continue;
        }

        if (connection->state == SENDING) {
            handle_sending(connection);
            continue;
        }

        if (connection->state == RECEIVING) {
            handle_receiving(server_state, connection, &error);
            if (error != NULL) {
                log_error(error);
                free_error(error);
            }

            continue;
        }

        disable_connection(connection, &error);
        if (error != NULL) {
            log_error(error);
            free_error(error);
        }

        log_fmt_msg(
                ERROR,
                "handle_poll_events poll_revents: '%d' fd: '%d'",
                connection->poll_revents,
                connection->socket_fd);
    }
}

int map_to_poll_fds(ArrayConnectionContext* connections, ArrayPollFd* poll_fds) {
    int active_connections_count = 0;

    for (int i = 0; i < connections->length; i++) {
        ConnectionContext* connection = &connections->items[i];

        if (connection->state != DISABLED)
            active_connections_count++;

        poll_fds->items[i].fd = connection->state == DISABLED ? -1 : connection->socket_fd;
        poll_fds->items[i].events = POLLIN;
    }

    return active_connections_count;
}

void map_revents(ArrayPollFd* poll_fds, ArrayConnectionContext* connections) {
    for (int i = 0; i < connections->length; i++) {
        ConnectionContext* connection = &connections->items[i];
        connection->poll_revents = poll_fds->items[i].revents;
    }
}

void init_server_state(
        ServerState* server_state,
        const char* socket_file_name,
        int backlog_length,
        Error** error) {
    server_state->counter = 0;
    server_state->is_running = true;

    server_state->socket_factory_fd = get_async_socket_factory(socket_file_name, backlog_length, error);
    if (*error != NULL)
        return;

    server_state->connections.length = backlog_length;
    server_state->connections.items = calloc(server_state->connections.length, sizeof(ConnectionContext));

    for (int i = 0; i < server_state->connections.length; i++)
        server_state->connections.items[i].state = DISABLED;
}

void run(ServerState* server_state) {
    ArrayPollFd poll_fds;
    poll_fds.length = server_state->connections.length;
    poll_fds.items = calloc(poll_fds.length, sizeof(PollFd));

    int prev_active_connections_count = 0;
    while (server_state->is_running) {
        int client_fd = try_accept_connection(server_state);
        if (client_fd >= 0)
            log_fmt_msg(INFO, "Accepted connection %d", client_fd);

        int active_connections_count = map_to_poll_fds(&server_state->connections, &poll_fds);
        if (prev_active_connections_count != active_connections_count) {
            prev_active_connections_count = active_connections_count;
            log_fmt_msg(INFO, "active_connections_count: %d", active_connections_count);
        }

        int ready_count = poll(poll_fds.items, poll_fds.length, 50);
        if (ready_count < 0) {
            Error* error = get_error_from_errno("poll error");
            log_error(error);
            free_error(error);
        }

        map_revents(&poll_fds, &server_state->connections);

        handle_poll_events(server_state);
    }

    log_fmt_msg(INFO, "Server was stopped");

    free(poll_fds.items);
}
