#include <app/manifest.h>
#include <app/private/metadata_parsing_internal.h>
#include <stdlib.h>
#include <string.h>

extern "C" {

bool app_id_is_valid(const char* id) {
    auto size = strlen(id);
    return size >= 5 && size <= APP_ID_LENGTH && app_metadata_validate_string(id, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.';
    });
}

}
