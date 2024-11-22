#include "StringUtils.h"
#include <cstring>

namespace tt {

int string_find_last_index(const char* text, size_t from_index, char find) {
    for (size_t i = from_index; i >= 0; i--) {
        if (text[i] == find) {
            return (int)i;
        }
    }
    return -1;
}

bool string_get_path_parent(const char* path, char* output) {
    int index = string_find_last_index(path, strlen(path) - 1, '/');
    if (index == -1) {
        return false;
    } else if (index == 0) {
        output[0] = '/';
        output[1] = 0x00;
        return true;
    } else {
        memcpy(output, path, index);
        output[index] = 0x00;
        return true;
    }
}

} // namespace
