/**
 * @brief key-value storage for general purpose.
 * Maps strings on a fixed set of data types.
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>

class Bundle {

private:

    typedef uint32_t Hash;

    typedef enum {
        TypeBool,
        TypeInt32,
        TypeString,
    } Type;

    typedef struct {
        const char* key;
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

    bool getBool(const std::string& key);
    int32_t getInt32(const std::string& key);
    std::string getString(const std::string& key);

    bool hasBool(const std::string& key);
    bool hasInt32(const std::string& key);
    bool hasString(const std::string& key);

    bool optBool(const std::string& key, bool& out);
    bool optInt32(const std::string& key, int32_t& out);
    bool optString(const std::string& key, std::string& out);

    void putBool(const std::string& key, bool value);
    void putInt32(const std::string& key, int32_t value);
    void putString(const std::string& key, const std::string& value);
};
