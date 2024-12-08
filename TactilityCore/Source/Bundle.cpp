#include "Bundle.h"

namespace tt {

bool Bundle::getBool(const std::string& key) const {
    return this->entries.find(key)->second.value_bool;
}

int32_t Bundle::getInt32(const std::string& key) const {
    return this->entries.find(key)->second.value_int32;
}

std::string Bundle::getString(const std::string& key) const {
    return this->entries.find(key)->second.value_string;
}

bool Bundle::hasBool(const std::string& key) const {
    auto entry = this->entries.find(key);
    return entry != std::end(this->entries) && entry->second.type == TypeBool;
}

bool Bundle::hasInt32(const std::string& key) const {
    auto entry = this->entries.find(key);
    return entry != std::end(this->entries) && entry->second.type == TypeInt32;
}

bool Bundle::hasString(const std::string& key) const {
    auto entry = this->entries.find(key);
    return entry != std::end(this->entries) && entry->second.type == TypeString;
}

bool Bundle::optBool(const std::string& key, bool& out) const {
    auto entry = this->entries.find(key);
    if (entry != std::end(this->entries) && entry->second.type == TypeBool) {
        out = entry->second.value_bool;
        return true;
    } else {
        return false;
    }
}

bool Bundle::optInt32(const std::string& key, int32_t& out) const {
    auto entry = this->entries.find(key);
    if (entry != std::end(this->entries) && entry->second.type == TypeInt32) {
        out = entry->second.value_int32;
        return true;
    } else {
        return false;
    }
}

bool Bundle::optString(const std::string& key, std::string& out) const {
    auto entry = this->entries.find(key);
    if (entry != std::end(this->entries) && entry->second.type == TypeString) {
        out = entry->second.value_string;
        return true;
    } else {
        return false;
    }
}

void Bundle::putBool(const std::string& key, bool value) {
    this->entries[key] = {
        .type = TypeBool,
        .value_bool = value,
        .value_string = ""
    };
}

void Bundle::putInt32(const std::string& key, int32_t value) {
    this->entries[key] = {
        .type = TypeInt32,
        .value_int32 = value,
        .value_string = ""
    };
}

void Bundle::putString(const std::string& key, const std::string& value) {
    this->entries[key] = {
        .type = TypeString,
        .value_bool = false,
        .value_string = value
    };
}

} // namespace
