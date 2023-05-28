#include <stdio.h>
#include "../sdk/config/config.h"
#include "../sdk/log/log.h"
#include "../sdk/socket/socket_utils.h"



int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "No config path or delay\n");
        return 1;
    }

    Error* error;

    char* socket_name = provide_socket_name(argv[1], &error);
    if (error != NULL) {
        log_error(error);
        return 1;
    }

    int socket_fd = connect_to_unix_socket(socket_name, &error);
    if (error != NULL) {
        log_error(error);
        return 1;
    }


    while (true) {

        sendall()
    }
}