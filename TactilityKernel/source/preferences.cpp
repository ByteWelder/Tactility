// SPDX-License-Identifier: Apache-2.0
#include <tactility/preferences.h>
#include <tactility/properties_file.h>
#include <tactility/paths.h>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

// Escapes '\\' and '\n' so a string value can never break properties_file's one-entry-per-line
// on-disk format, regardless of its content.
std::string escape(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (c == '\\') {
            result += "\\\\";
        } else if (c == '\n') {
            result += "\\n";
        } else {
            result += c;
        }
    }
    return result;
}

std::string unescape(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); i++) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            i++;
            result += (value[i] == 'n') ? '\n' : value[i];
        } else {
            result += value[i];
        }
    }
    return result;
}

// Splits a tagged value ("b:1", "i32:42", "i64:123", "s:escaped text") into its type tag and
// raw payload. Returns false if there's no ':' separator (malformed/missing).
bool split_tag(const std::string& tagged_value, std::string& tag, std::string& raw_value) {
    size_t colon = tagged_value.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    tag = tagged_value.substr(0, colon);
    raw_value = tagged_value.substr(colon + 1);
    return true;
}

bool ensure_directory(const std::string& path) {
    struct stat info {};
    if (stat(path.c_str(), &info) == 0) {
        return (info.st_mode & S_IFMT) == S_IFDIR;
    }
    return mkdir(path.c_str(), 0777) == 0;
}

// mkdir -p.
bool ensure_directory_recursive(const std::string& path) {
    for (size_t index = path.find('/', 1); index != std::string::npos; index = path.find('/', index + 1)) {
        if (!ensure_directory(path.substr(0, index))) {
            return false;
        }
    }
    return ensure_directory(path);
}

} // namespace

// Definition of the opaque handle declared in tactility/preferences.h - C callers only ever
// see it through a Preferences* pointer, never its members. Backed by a PropertiesFile
// (tactility/properties_file.h) rather than its own file I/O - each value is stored as a
// tagged string ("b:1", "i32:42", "i64:123", "s:escaped text") since PropertiesFile only knows
// about plain strings.
struct Preferences {
    PropertiesFile* file;
};

namespace {

// Grow-and-retry: properties_file_get() needs a bounded buffer, and a string preference's
// value (unlike bool/int32/int64's short encodings) can be arbitrarily long.
bool try_get_tagged(const PropertiesFile* file, const char* key, std::string& tag, std::string& raw_value) {
    std::vector<char> buffer(32);
    while (true) {
        error_t error = properties_file_get(file, key, buffer.data(), buffer.size());
        if (error == ERROR_NONE) {
            return split_tag(std::string(buffer.data()), tag, raw_value);
        }
        if (error == ERROR_NOT_FOUND) {
            return false;
        }
        buffer.resize(buffer.size() * 2);
    }
}

} // namespace

extern "C" {

Preferences* preferences_open(const char* path) {
    PropertiesFile* file = properties_file_open(path);
    if (file == nullptr) {
        return nullptr;
    }

    auto* preferences = new (std::nothrow) Preferences { file };
    if (preferences == nullptr) {
        properties_file_close(file);
        return nullptr;
    }
    return preferences;
}

void preferences_close(Preferences* preferences) {
    properties_file_close(preferences->file);
    delete preferences;
}

bool preferences_has_bool(const Preferences* preferences, const char* key) {
    std::string tag, raw_value;
    return try_get_tagged(preferences->file, key, tag, raw_value) && tag == "b";
}

bool preferences_has_int32(const Preferences* preferences, const char* key) {
    std::string tag, raw_value;
    return try_get_tagged(preferences->file, key, tag, raw_value) && tag == "i32";
}

bool preferences_has_int64(const Preferences* preferences, const char* key) {
    std::string tag, raw_value;
    return try_get_tagged(preferences->file, key, tag, raw_value) && tag == "i64";
}

bool preferences_has_string(const Preferences* preferences, const char* key) {
    std::string tag, raw_value;
    return try_get_tagged(preferences->file, key, tag, raw_value) && tag == "s";
}

bool preferences_opt_bool(const Preferences* preferences, const char* key, bool* out_value) {
    std::string tag, raw_value;
    if (!try_get_tagged(preferences->file, key, tag, raw_value) || tag != "b") {
        return false;
    }
    *out_value = (raw_value == "1");
    return true;
}

bool preferences_opt_int32(const Preferences* preferences, const char* key, int32_t* out_value) {
    std::string tag, raw_value;
    if (!try_get_tagged(preferences->file, key, tag, raw_value) || tag != "i32") {
        return false;
    }
    *out_value = static_cast<int32_t>(std::strtol(raw_value.c_str(), nullptr, 10));
    return true;
}

bool preferences_opt_int64(const Preferences* preferences, const char* key, int64_t* out_value) {
    std::string tag, raw_value;
    if (!try_get_tagged(preferences->file, key, tag, raw_value) || tag != "i64") {
        return false;
    }
    *out_value = static_cast<int64_t>(std::strtoll(raw_value.c_str(), nullptr, 10));
    return true;
}

error_t preferences_opt_string(const Preferences* preferences, const char* key, char* out_value, size_t out_value_size) {
    std::string tag, raw_value;
    if (!try_get_tagged(preferences->file, key, tag, raw_value) || tag != "s") {
        return ERROR_NOT_FOUND;
    }
    std::string value = unescape(raw_value);
    if (value.size() + 1 > out_value_size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    std::memcpy(out_value, value.c_str(), value.size() + 1);
    return ERROR_NONE;
}

void preferences_put_bool(Preferences* preferences, const char* key, bool value) {
    properties_file_set(preferences->file, key, value ? "b:1" : "b:0");
}

void preferences_put_int32(Preferences* preferences, const char* key, int32_t value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "i32:%" PRId32, value);
    properties_file_set(preferences->file, key, buffer);
}

void preferences_put_int64(Preferences* preferences, const char* key, int64_t value) {
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "i64:%" PRId64, value);
    properties_file_set(preferences->file, key, buffer);
}

void preferences_put_string(Preferences* preferences, const char* key, const char* value) {
    std::string tagged = "s:" + escape(value);
    properties_file_set(preferences->file, key, tagged.c_str());
}

} // extern "C"
