#ifndef MYINIT_UTILS_H
#define MYINIT_UTILS_H


#include <bits/types/FILE.h>
#include "../error/error.h"

char* read_from_path(const char* path, long* output_buffer_length, Error** error);

char* read_file(FILE* file, long* output_buffer_length, Error** error);

bool is_abs_path(const char* path);

#endif
