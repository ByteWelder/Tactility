// SPDX-License-Identifier: Apache-2.0
#include <app/manifest.h>
#include <app/package_manifest.h>
#include <app/private/package_manifest_parsing.h>

#include <tactility/log.h>

#include <cctype>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

constexpr auto* TAG = "app_metadata";

bool app_package_manifest_validate_string(const std::string& value, bool (*is_valid_char)(char)) {
    for (char c: value) {
        if (!is_valid_char(c)) {
            return false;
        }
    }
    return true;
}

namespace {

#define validate_string app_package_manifest_validate_string

std::string trim(const std::string& value) {
    constexpr auto* whitespace = " \t\r\n";
    auto start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    auto end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

/** Validates a comma-separated list: non-empty, no leading/trailing/double commas (which would
 * produce an empty item), and every item passing @a is_valid_item. */
bool validate_csv_list(const std::string& value, bool (*is_valid_item)(const std::string&)) {
    if (value.empty()) {
        return false;
    }
    size_t start = 0;
    while (true) {
        auto comma = value.find(',', start);
        auto end = comma == std::string::npos ? value.size() : comma;
        if (end == start || !is_valid_item(value.substr(start, end - start))) {
            return false;
        }
        if (comma == std::string::npos) {
            return true;
        }
        start = comma + 1;
    }
}

/** manifest.properties format: flat "key=value" lines, "#" lines are comments, blank lines are
 * skipped. Deliberately a local, minimal re-implementation rather than depending on Tactility's
 * file::loadPropertiesFile() - app-module (like every other kernel module) may not depend upward
 * on the Tactility layer. */
bool load_properties(const std::string& path, std::map<std::string, std::string>& out_properties) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto trimmed_line = trim(line);

        if (trimmed_line.empty() || trimmed_line.starts_with("#")) {
            continue;
        }

        auto separator_index = trimmed_line.find('=');
        if (separator_index == std::string::npos) {
            LOG_E(TAG, "Failed to parse manifest line (skipped): %s", trimmed_line.c_str());
            continue;
        }

        auto key = trim(trimmed_line.substr(0, separator_index));
        auto value = trim(trimmed_line.substr(separator_index + 1));
        out_properties[key] = value;
    }

    return true;
}

} // namespace

bool app_package_manifest_get_value(const std::map<std::string, std::string>& properties, const std::string& key, std::string& out_value) {
    const auto iterator = properties.find(key);
    if (iterator == properties.end()) {
        LOG_E(TAG, "Failed to find %s in manifest", key.c_str());
        return false;
    }
    out_value = iterator->second;
    return true;
}

bool app_package_manifest_is_valid_format_version(const std::string& version) {
    return !version.empty() && validate_string(version, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.';
    });
}

bool app_package_manifest_is_valid_version_name(const std::string& version) {
    return !version.empty() && version.size() <= PACKAGE_MANIFEST_VERSION_NAME_LENGTH && validate_string(version, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.' || c == '-' || c == '_';
    });
}

bool app_package_manifest_is_valid_version_code(const std::string& version) {
    // 20 digits is the maximum decimal width of uint64_t.
    return !version.empty() && version.size() <= 20 && validate_string(version, [](char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    });
}

bool app_package_manifest_is_valid_bool(const std::string& value) {
    return value == "true" || value == "false";
}

bool app_package_manifest_is_valid_device_id_list(const std::string& value) {
    return validate_csv_list(value, [](const std::string& item) {
        bool has_alnum = false;
        for (char c: item) {
            if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
                has_alnum = true;
            } else if (c != '-') {
                return false;
            }
        }
        return has_alnum;
    });
}

bool app_package_manifest_is_valid_binary_name(const std::string& value) {
    return !value.empty() && value.size() <= APP_MANIFEST_BINARY_LENGTH && validate_string(value, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.' || c == '_' || c == '-';
    });
}

bool app_package_manifest_copy_bounded(char* dest, size_t dest_size, const std::string& value) {
    if (value.size() >= dest_size) {
        return false;
    }
    memcpy(dest, value.c_str(), value.size() + 1);
    return true;
}

error_t app_package_manifest_parse(const char* path, struct PackageManifest* out_package, struct AppManifestBinding* out_bindings, size_t bindings_capacity) {
    LOG_I(TAG, "Parsing manifest %s", path);

    // requires_device_id is optional; zeroing here (rather than relying on the caller) guarantees
    // it reads back as empty ("unrestricted") when the manifest omits it.
    *out_package = {};
    for (size_t i = 0; i < bindings_capacity; i++) {
        out_bindings[i] = {};
    }

    std::map<std::string, std::string> properties;
    if (!load_properties(path, properties)) {
        LOG_E(TAG, "Failed to load manifest at %s", path);
        return ERROR_NOT_FOUND;
    }

    std::string format_version;
    if (!app_package_manifest_get_value(properties, "manifest.version", format_version)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (format_version == "0.2") {
        return package_manifest_parse_v2(properties, *out_package, out_bindings, bindings_capacity);
    } else if (format_version == "0.3") {
        return package_manifest_parse_v3(properties, *out_package, out_bindings, bindings_capacity);
    } else {
        LOG_E(TAG, "Unsupported manifest.version: %s", format_version.c_str());
        return ERROR_INVALID_ARGUMENT;
    }
}

error_t app_package_manifest_parse_into(const char* path, PackageManifest& out_package, std::vector<AppManifestBinding, tt::OptExternalAllocator<AppManifestBinding>>& out_bindings) {
    error_t result = app_package_manifest_parse(path, &out_package, nullptr, 0);
    if (result != ERROR_NONE) {
        return result;
    }
    out_bindings.resize(out_package.app_manifest_count);
    return app_package_manifest_parse(path, &out_package, out_bindings.data(), out_bindings.size());
}
