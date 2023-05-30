#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "file_utils.h"

long get_file_size(FILE* file) {
    long old_position = ftell(file);

    fseek(file, 0, SEEK_END);
    long result = ftell(file);

    fseek(file, old_position, SEEK_SET);

    return result;
}

char* read_from_path(const char* path, long* output_buffer_length, Error** error) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        *error = attach_error(get_error_from_errno("read_from_path"), get_error_from_message("opening path: %s", path));
        return NULL;
    }

    char* result = read_file(file, output_buffer_length, error);

    if (fclose(file) != 0)
        *error = attach_error(get_error_from_errno("close"), *error);

    if (*error != NULL)
        return NULL;

    return result;
}

char* read_file(FILE* file, long* output_buffer_length, Error** error) {
    long text_buffer_length = get_file_size(file) + 1;
    *output_buffer_length  = text_buffer_length;

    char* text = (char*)malloc(text_buffer_length);

    fread(text, 1, text_buffer_length, file);
    if (ferror(file)) {
        *error = get_error_from_errno("read_file");
        free(text);

        return NULL;
    }

    text[text_buffer_length - 1] = '\0';

    return text;
}

bool is_abs_path(const char* path) {
    return path[0] == '/';
}
