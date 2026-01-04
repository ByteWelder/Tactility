#pragma once

#include <cJSON.h>
#include <string>
#include <vector>

#include <Tactility/Logger.h>

namespace tt::json {

class Reader {

    const cJSON* root;
    Logger logger = Logger("json::Reader");

public:

    explicit Reader(const cJSON* root) : root(root) {}

    bool readString(const char* key, std::string& output) const {
        const auto* child = cJSON_GetObjectItemCaseSensitive(root, key);
        if (!cJSON_IsString(child)) {
            logger.error("{} is not a string", key);
            return false;
        }
        output = cJSON_GetStringValue(child);
        return true;
    }

    bool readInt32(const char* key, int32_t& output) const {
        double buffer;
        if (!readNumber(key, buffer)) {
            return false;
        }
        output = static_cast<int32_t>(buffer);
        return true;
    }

    bool readInt(const char* key, int& output) const {
        double buffer;
        if (!readNumber(key, buffer)) {
            return false;
        }
        output = buffer;
        return true;
    }

    bool readNumber(const char* key, double& output) const {
        const auto* child = cJSON_GetObjectItemCaseSensitive(root, key);
        if (!cJSON_IsNumber(child)) {
            logger.error("{} is not a number", key);
            return false;
        }
        output = cJSON_GetNumberValue(child);
        return true;
    }

    bool readStringArray(const char* key, std::vector<std::string>& output) const {
        const auto* child = cJSON_GetObjectItemCaseSensitive(root, key);
        if (!cJSON_IsArray(child)) {
            logger.error("{} is not an array", key);
            return false;
        }
        const auto size = cJSON_GetArraySize(child);
        logger.info("Processing {} array children", size);
        output.resize(size);
        for (int i = 0; i < size; ++i) {
            const auto string_json = cJSON_GetArrayItem(child, i);
            if (!cJSON_IsString(string_json)) {
                logger.error("array child of %s is not a string", key);
                return false;
            }
            output[i] = cJSON_GetStringValue(string_json);
        }
        return true;
    }
};

}