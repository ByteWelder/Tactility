#include "Tactility/file/PropertiesFile.h"

#include <Tactility/StringUtils.h>
#include <Tactility/file/File.h>

namespace tt::file {

static auto TAG = "PropertiesFile";

bool getKeyValuePair(const std::string& input, std::string& key, std::string& value) {
    auto index = input.find('=');
    if (index == std::string::npos) {
        return false;
    }
    key = input.substr(0, index);
    value = input.substr(index + 1);
    return true;
}

bool loadPropertiesFile(const std::string& filePath, std::function<void(const std::string& key, const std::string& value)> callback) {
    auto input = readString(filePath);
    if (input == nullptr) {
        TT_LOG_E(TAG, "Failed to read file contents of %s", filePath.c_str());
        return false;
    }

    const char* input_start = reinterpret_cast<const char*>(input.get());
    std::string input_string;
    input_string.assign(input_start);

    uint16_t line_count = 0;
    string::split(input_string, "\n", [&line_count, &filePath, &callback](auto token) {
        line_count++;
        std::string key, value;
        auto trimmed_token = string::trim(token, " \t");
        if (!trimmed_token.starts_with("#") ) {
            if (getKeyValuePair(token, key, value)) {
                std::string trimmed_key = string::trim(key, " \t");
                std::string trimmed_value = string::trim(value, " \t");
                callback(trimmed_key, trimmed_value);
            } else {
                TT_LOG_E(TAG, "Failed to parse line %d of %s", line_count, filePath.c_str());
            }
        }
    });

    return true;
}

bool loadPropertiesFile(const std::string& filePath, std::map<std::string, std::string>& outProperties) {
    return loadPropertiesFile(filePath, [&outProperties](const std::string& key, const std::string& value) {
        outProperties[key] = value;
    });
}

}
