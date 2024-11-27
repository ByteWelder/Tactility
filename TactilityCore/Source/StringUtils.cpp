#include "StringUtils.h"
#include <cstring>
#include <iostream>
#include <sstream>

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

std::vector<std::string> string_split(const std::string&input, const std::string&delimiter) {
    size_t token_index = 0;
    size_t delimiter_index;
    const size_t delimiter_length = delimiter.length();
    std::string token;
    std::vector<std::string> result;

    while ((delimiter_index = input.find(delimiter, token_index)) != std::string::npos) {
        token = input.substr(token_index, delimiter_index - token_index);
        token_index = delimiter_index + delimiter_length;
        result.push_back(token);
    }

    auto end_token = input.substr(token_index);
    if (!end_token.empty()) {
        result.push_back(end_token);
    }

    return result;
}

std::string string_join(const std::vector<std::string>& input, const std::string& delimiter) {
    std::stringstream stream;
    size_t size = input.size();

    if (size == 0) {
        return "";
    } else if (size == 1) {
        return input.front();
    } else {
        auto iterator = input.begin();
        while (iterator != input.end()) {
            stream << *iterator;
            iterator++;
            if (iterator != input.end()) {
                stream << delimiter;
            }
        }
    }

    return stream.str();
}

} // namespace
