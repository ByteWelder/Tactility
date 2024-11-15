#pragma once

#include <cstdint>
#include <cstddef>

class Preferences {
private:
    const char* namespace_;

public:
    explicit Preferences(const char* namespace_) {
        this->namespace_ = namespace_;
    }

    bool hasBool(const char* key);
    bool hasInt32(const char* key);
    bool hasString(const char* key);

    bool optBool(const char* key, bool* out);
    bool optInt32(const char* key, int32_t* out);
    bool optString(const char* key, char* out, size_t* size);

    void putBool(const char* key, bool value);
    void putInt32(const char* key, int32_t value);
    void putString(const char* key, const char* string);
};
