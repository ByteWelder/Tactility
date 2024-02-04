#include "string_utils.h"
#include <string.h>

int tt_string_find_last_index(const char* text, size_t from_index, char find) {
    for (size_t i = from_index; i >= 0; i--) {
        if (text[i] == find) {
            return (int)i;
        }
    }
    return -1;
}

bool tt_string_get_path_parent(const char* path, char* output) {
    int index = tt_string_find_last_index(path, strlen(path) - 1, '/');
    if (index == -1) {
        return false;
    } else {
        memcpy(output, path, index);
        output[index] = 0x00;
        return true;
    }
}
