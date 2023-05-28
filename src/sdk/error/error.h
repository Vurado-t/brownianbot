#ifndef LCK_ERROR_HANDLING_H
#define LCK_ERROR_HANDLING_H


#include <stdbool.h>

typedef struct Error Error;

struct Error {
    int code;
    char* message;
    Error* inner;
};

Error* get_error_from_errno(const char* prefix_message);

Error* get_error_from_message(const char* fmt, ...);

Error* attach_error(Error* current, Error* inner);

void free_error(Error* error);


#endif
