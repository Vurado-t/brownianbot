#include <stddef.h>
#include <malloc.h>
#include "config.h"
#include "../file/file_utils.h"

char* provide_socket_name(const char* config_path, Error** error) {
    long buffer_length;
    char* config_bytes = read_from_path(config_path, &buffer_length, error);
    if (*error != NULL)
        return NULL;

    char* result = malloc(buffer_length);
    for (int i = 0; i < buffer_length; i++) {
        if (config_bytes[i] == '\0' || config_bytes[i] == '\n') {
            result[i] = '\0';
            break;
        }

        result[i] = config_bytes[i];
    }

    free(config_bytes);

    if (!is_abs_path(result)) {
        *error = get_error_from_message("socket path must be absolute: '%s'", result);
        free(result);
        return NULL;
    }

    return result;
}
