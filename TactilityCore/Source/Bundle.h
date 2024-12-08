/**
 * @brief key-value storage for general purpose.
 * Maps strings on a fixed set of data types.
 */
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace tt {

class Bundle {

private:

    typedef uint32_t Hash;

    typedef enum {
        TypeBool,
        TypeInt32,
        TypeString,
    } Type;

    typedef struct {
        Type type;
        union {
            bool value_bool;
            int32_t value_int32;
        };
        std::string value_string;
    } Value;

    std::unordered_map<std::string, Value> entries;

public:

    Bundle() = default;

    Bundle(const Bundle& bundle) {
        this->entries = bundle.entries;
    }

    bool getBool(const std::string& key) const;
    int32_t getInt32(const std::string& key) const;
    std::string getString(const std::string& key) const;

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

} // namespace
