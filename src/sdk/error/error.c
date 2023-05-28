#include "error.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>
#include <errno.h>

#define MESSAGE_BUFFER_LENGTH 256

Error* construct_error(int code, Error* inner, const char* fmt, ...) {
    Error* error = malloc(sizeof(Error));

    error->code = code;
    error->inner = inner;

    error->message = malloc(MESSAGE_BUFFER_LENGTH);

    va_list params;
    va_start(params, fmt);

    vsnprintf(error->message, MESSAGE_BUFFER_LENGTH, fmt, params);

    va_end(params);

    return error;
}

Error* get_error_from_errno(const char* prefix_message) {
    return construct_error(errno, NULL, "%s (errno: %s)", prefix_message, strerror(errno));
}

Error* get_error_from_message(const char* fmt, ...) {
    va_list params;
    va_start(params, fmt);

    Error* error = construct_error(1, NULL, fmt, params);

    va_end(params);

    return error;
}

Error* attach_error(Error* current, Error* inner) {
    current->inner = inner;
    return current;
}

void free_error(Error* error) { // NOLINT(misc-no-recursion)
    if (error == NULL) {
        return;
    }

    free_error(error->inner);

    free(error->message);
    free(error);
}
