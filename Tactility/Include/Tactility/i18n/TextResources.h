#pragma once

#include <string>
#include <vector>

namespace tt::i18n {

/**
 * Holds localized text data.
 *
 * It is used with data generated from Translations/ with the python generation scripts.
 * It's used with a header file that specifies the indexes, and generated text files (.i18n)
 */
class TextResources {

    std::vector<std::string> data;
    std::string path;
    static std::string ERROR_RESULT;

public:
    /**
     * @param[in] path
     */
    TextResources(const std::string& path) : path(path) {}

    const std::string& get(const int index) const {
        if (index < data.size()) {
            return data[index];
        } else {
            return ERROR_RESULT;
        }
    }

    template <typename EnumType>
    const std::string& get(EnumType value) const { return get(static_cast<int>(value)); }

    const std::string& operator[](const int index) const { return get(index); }

    template <typename EnumType>
    const std::string& operator[](const EnumType index) const { return get(index); }

    /**
     * Load or reload an i18n file with the system's current locale settings.
     * @return true on success
     */
    bool load();
};

}