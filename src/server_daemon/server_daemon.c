#include "../sdk/log/log.h"
#include "../sdk/server/server.h"
#include "../sdk/file/file_utils.h"
#include <stddef.h>
#include <signal.h>
#include <stdlib.h>

ServerState* server_state;

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

int main(int argc, char** argv) {
    if (argc < 2) {
        log_fmt_msg(ERROR, "No config path");
        return 1;
    }

    char* config_path = argv[1];
    if (!is_abs_path(config_path)) {
        log_fmt_msg(ERROR, "Config path must be absolute");
        return 1;
    }

    Error* error;

    setup_signal_handling();

    char* socket_path = provide_socket_name(config_path, &error);
    if (error != NULL) {
        log_error(error);
        return error->code;
    }

    log_fmt_msg(INFO, "socket path: '%s'", socket_path);

    server_state = malloc(sizeof(ServerState));
    init_server_state(server_state, socket_path, 100, &error);
    if (error != NULL) {
        log_error(error);
        return error->code;
    }

    free(socket_path);

    run(server_state);

    return 0;
}
