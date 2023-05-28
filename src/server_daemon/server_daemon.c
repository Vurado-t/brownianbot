#include "../sdk/log/log.h"
#include "../sdk/server/server.h"
#include "../sdk/file/file_utils.h"
#include <stddef.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

#define LOG_PATH "/tmp/vurado/brownianbot/server.log"

ServerState* server_state;

void init_log(Error** error) {
    *error = NULL;

    FILE* log_file = fopen(LOG_PATH, "a+");
    if (log_file == NULL) {
        *error = get_error_from_errno("init_log");
        return;
    }


    init_std_log(log_file, error);
    if (*error != NULL)
        return;

    if (fclose(log_file) != 0) {
        *error = get_error_from_errno("init_log close log_file");
        return;
    }
}

void close_all_files() {
    for (int i = 0; i < FD_SETSIZE; i++)
        close(i);
}

void detach_terminal() {
    if (fork() != 0)
        exit(0);

    if (setsid() < 0)
        exit(errno);

    log_fmt_msg(INFO, "Daemon started with pid: %d", getpid());
}

void handle_signals(int signum) {
    switch (signum) {
        case SIGINT:
            server_state->is_running = false;
            break;
        default:
            break;
    }
}

void setup_signal_handling() {
    struct sigaction action;
    action.sa_handler = handle_signals;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGHUP, &action, NULL);
}

void turn_into_daemon() {
    close_all_files();
    detach_terminal();
    setup_signal_handling();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "No config path");
        return 1;
    }

    char* config_path = argv[1];
    if (!is_abs_path(config_path)) {
        fprintf(stderr, "Config path must be absolute");
        return 1;
    }

    printf("Logs are in %s\n", LOG_PATH);

    Error* error;

    init_log(&error);
    if (error != NULL) {
        log_error(error);
        exit(error->code);
    }

    turn_into_daemon();

    char* socket_path = provide_socket_name(config_path, &error);
    if (error != NULL) {
        log_error(error);
        return error->code;
    }

    log_fmt_msg(INFO, "socket path: '%s'", socket_path);

    server_state = malloc(sizeof(ServerState));
    init_server_state(server_state, socket_path, 1000, &error);
    if (error != NULL) {
        log_error(error);
        return error->code;
    }

    free(socket_path);

    run(server_state);

    return 0;
}
