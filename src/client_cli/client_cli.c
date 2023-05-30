#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "../sdk/config/config.h"
#include "../sdk/log/log.h"
#include "../sdk/socket/socket_utils.h"

#define NS_IN_SECOND 1000000000

typedef struct {
    long summary_delay_ns;
} ClientStatistics;

long sleep_ns(long ns) {
    struct timespec timespec = (struct timespec){.tv_sec = ns / NS_IN_SECOND, .tv_nsec = ns % NS_IN_SECOND};
    struct timespec remain = (struct timespec){.tv_sec = 0, .tv_nsec = 0};
    nanosleep(&timespec, &remain);

    return ns - (remain.tv_sec * NS_IN_SECOND + remain.tv_nsec);
}

long send_request(int socket_fd, long request, Error** error) {
    *error = NULL;

    char str[32];
    snprintf(str, 32, "%li\n", request);

    send_string(socket_fd, str, error);
    if (*error != NULL)
        return 0;

    char recv_str[32];
    char* response = recv_line(socket_fd, recv_str, 32, error);
    if (*error != NULL)
        return 0;

    long response_number = atoi(response); // NOLINT(cert-err34-c)

    return response_number;
}

ClientStatistics* run_client(const char* socket_name, Error** error, long delay_ns, int delay_index) {
    *error = NULL;

    ClientStatistics* statistics = calloc(1, sizeof(ClientStatistics));
    statistics->summary_delay_ns = 0;

    int socket_fd = connect_to_unix_socket(socket_name, error);
    if (error != NULL) {
        free(statistics);
        return NULL;
    }

    long file_offset = 0;
    long number = 0;
    char c;
    bool is_whitespace_sequence = false;

    while ((c = (char)getc(stdin)) != EOF) {
        file_offset++;
        if (file_offset % 256 == delay_index && statistics->summary_delay_ns < 1)
            statistics->summary_delay_ns += sleep_ns(delay_ns);

        if (c == '\n') {
            if (is_whitespace_sequence)
                continue;

            log_fmt_msg(INFO, "Sent request: %li", number);

            long response = send_request(socket_fd, number, error);
            if (*error != NULL) {
                free(statistics);
                return NULL;
            }

            log_fmt_msg(INFO, "Got response: %li", response);

            number = 0;
            is_whitespace_sequence = true;
        }
        else if (isdigit(c)) {
            number = number * 10 + c - '0';
            is_whitespace_sequence = false;
        }
        else {
            *error = get_error_from_message("Non digit symbol: %c", c);
            free(statistics);
            return NULL;
        }
    }

    return statistics;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "No config path or delay\n");
        return 1;
    }

    char* config_path = argv[1];
    long delay_ms = atoi(argv[2]); // NOLINT(cert-err34-c)

    log_fmt_msg(INFO, "Config path: %s", config_path);
    log_fmt_msg(INFO, "Delay milliseconds: %li", delay_ms);

    Error* error;

    char* socket_name = provide_socket_name(argv[1], &error);
    if (error != NULL) {
        log_error(error);
        return 1;
    }

    log_fmt_msg(INFO, "Socket name: %s", socket_name);

    srand(time(NULL)); // NOLINT(cert-msc51-cpp)

    ClientStatistics* statistics = run_client(
            socket_name,
            &error,
            delay_ms * 1000000,
            rand() % 256); // NOLINT(cert-msc50-cpp)

    if (error != NULL) {
        log_error(error);
        return error->code;
    }

    log_fmt_msg(INFO, "Summary delay ns %li", statistics->summary_delay_ns);

    return 0;
}