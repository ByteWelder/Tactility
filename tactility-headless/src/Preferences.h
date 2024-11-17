#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

class Preferences {
private:
    const char* namespace_;

public:
    explicit Preferences(const char* namespace_) {
        this->namespace_ = namespace_;
    }

    bool hasBool(const std::string& key) const;
    bool hasInt32(const std::string& key) const;
    bool hasString(const std::string& key) const;

    bool optBool(const std::string& key, bool& out) const;
    bool optInt32(const std::string& key, int32_t& out) const;
    bool optString(const std::string& key, std::string& out) const;

    void putBool(const std::string& key, bool value);
    void putInt32(const std::string& key, int32_t value);
    void putString(const std::string& key, const std::string& value);
};
