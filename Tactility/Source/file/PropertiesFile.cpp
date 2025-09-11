#include "Tactility/file/PropertiesFile.h"

#include <Tactility/StringUtils.h>
#include <Tactility/file/File.h>
#include <Tactility/file/FileLock.h>

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
    return file::withLock<bool>(filePath, [&filePath, &callback] {
        TT_LOG_I(TAG, "Reading properties file %s", filePath.c_str());
        uint16_t line_count = 0;
        std::string key_prefix = "";
        return readLines(filePath, true, [&key_prefix, &line_count, &filePath, &callback](const std::string& line) {
            line_count++;
            std::string key, value;
            auto trimmed_line = string::trim(line, " \t");
            if (!trimmed_line.starts_with("#")) {
                if (trimmed_line.starts_with("[")) {
                    key_prefix = trimmed_line;
                } else {
                    if (getKeyValuePair(trimmed_line, key, value)) {
                       std::string trimmed_key = key_prefix + string::trim(key, " \t");
                       std::string trimmed_value = string::trim(value, " \t");
                       callback(trimmed_key, trimmed_value);
                   } else {
                       TT_LOG_E(TAG, "Failed to parse line %d of %s", line_count, filePath.c_str());
                   }
                }
            }
        });
    });
}

bool loadPropertiesFile(const std::string& filePath, std::map<std::string, std::string>& outProperties) {
    return loadPropertiesFile(filePath, [&outProperties](const std::string& key, const std::string& value) {
        outProperties[key] = value;
    });
}

bool savePropertiesFile(const std::string& filePath, const std::map<std::string, std::string>& properties) {
    return file::withLock<bool>(filePath, [filePath, &properties] {
        TT_LOG_I(TAG, "Saving properties file %s", filePath.c_str());

        FILE* file = fopen(filePath.c_str(), "w");
        if (file == nullptr) {
            TT_LOG_E(TAG, "Failed to open %s", filePath.c_str());
            return false;
        }

        for (const auto& [key, value]: properties) { fprintf(file, "%s=%s\n", key.c_str(), value.c_str()); }

        fclose(file);
        return true;
    });
}

}
