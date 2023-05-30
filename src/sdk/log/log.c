#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"

static const char* LOG_LEVEL_NAME[] = {"INFO", "ERROR"};

void init_std_log(FILE* file, Error** error) {
    *error = NULL;

    int log_fd = fileno(file);
    dup2(log_fd, 1);
    dup2(log_fd, 2);
}

void log_fmt_msg(LogLevel level, const char* fmt, ...) {
    printf("[%s] [%li] ", LOG_LEVEL_NAME[level], time(NULL));

    va_list params;
    va_start(params, fmt);

    vprintf(fmt, params);

    va_end(params);

    printf("\n");

    fflush(stdout);
    fflush(stderr);
}

void log_error(const Error* error) {
    fprintf(stderr, "[ERROR] [%li] [%d] %s\n", time(NULL), error->code, error->message);
    fflush(stdout);
    fflush(stderr);
}
