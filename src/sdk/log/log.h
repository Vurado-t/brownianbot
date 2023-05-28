#include <bits/types/FILE.h>
#include "../error/error.h"
#include "../config/config.h"

#ifndef MYINIT_LOG_H
#define MYINIT_LOG_H


typedef enum {
    INFO,
    ERROR
} LogLevel;

void init_std_log(FILE* file, Error** error);

void log_error(const Error* error);

void log_fmt_msg(LogLevel level, const char* fmt, ...);


#endif
