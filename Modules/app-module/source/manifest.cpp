#include <app/manifest.h>
#include <app/private/package_manifest_parsing.h>
#include <stdlib.h>
#include <string.h>

extern "C" {

bool app_manifest_id_is_valid(const char* id) {
    auto size = strlen(id);
    return size >= 5 && size <= APP_MANIFEST_ID_LENGTH && app_package_manifest_validate_string(id, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.';
    });
}

bool app_manifest_name_is_valid(const char* name) {
    auto size = strlen(name);
    return size >= 2 && size <= APP_MANIFEST_NAME_LENGTH && app_package_manifest_validate_string(name, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == ' ' || c == '-';
    });
}

bool app_manifest_stack_size_is_valid(const char* value) {
    // 10 digits is the maximum decimal width of uint32_t.
    auto size = strlen(value);
    return size > 0 && size <= 10 && app_package_manifest_validate_string(value, [](char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    });
}

}
