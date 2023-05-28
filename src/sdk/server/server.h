#ifndef BROWNIANBOT_SERVER_H
#define BROWNIANBOT_SERVER_H


#include "../misc/generic.h"
#include "../error/error.h"
#include <stdbool.h>

#define SEND_BUFFER_LENGTH 256
#define RECEIVE_BUFFER_LENGTH 256

typedef enum {
    DISABLED,
    RECEIVING,
    SENDING
} ConnectionState;

typedef struct {
    ConnectionState state;
    int socket_fd;
    short poll_revents;
    char send_buffer[SEND_BUFFER_LENGTH];
    int sent_count;
    int response_length;
    char receive_buffer[RECEIVE_BUFFER_LENGTH];
    int request_current_offset;
    int request_start_offset;
} ConnectionContext;

DEFINE_ARRAY(ConnectionContext);

typedef struct pollfd PollFd;
DEFINE_ARRAY(PollFd);

typedef struct {
    long counter;
    bool is_running;
    int socket_factory_fd;
    ArrayConnectionContext connections;
} ServerState;

void init_server_state(
        ServerState* server_state,
        const char* socket_file_name,
        int backlog_length,
        Error** error);

void run(ServerState* server_state);


#endif
