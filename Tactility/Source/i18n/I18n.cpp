#include "Tactility/i18n/I18n.h"
#include "Tactility/file/FileLock.h"

#include <cstring>
#include <vector>
#include <Tactility/file/File.h>

namespace tt::i18n {

constexpr auto* TAG = "I18n";
static std::string ERROR_RESULT = "TRANSLATION_ERROR";

class IndexedTextImplementation : IndexedText {

    std::vector<std::string> data;

public:

    explicit IndexedTextImplementation(std::vector<std::string> data) : data(std::move(data)) {}

    const std::string& get(const int index) const override {
        if (index < data.size()) {
            return data[index];
        } else {
            return ERROR_RESULT;
        }
    }
};

static std::string getDesiredLocale() {
    // TODO: Implement locale settings
    return "en-GB";
}

static std::string getFallbackLocale() {
    // TODO: Implement locale settings
    return "en-GB";
}

static std::string getI18nDataFilePath(const std::string& path) {
    auto locale = getDesiredLocale();
    auto desired_file_path = std::format("{}/{}.i18n", path, locale);
    if (file::isFile(desired_file_path)) {
        return desired_file_path;
    } else {
        TT_LOG_W(TAG, "Translations not found for %s at %s", locale.c_str(), desired_file_path.c_str());
    }

    auto fallback_locale = getFallbackLocale();
    auto fallback_file_path = std::format("{}/{}.i18n", path, getFallbackLocale());
    if (file::isFile(fallback_file_path)) {
        return fallback_file_path;
    } else {
        TT_LOG_W(TAG, "Fallback translations not found for %s at %s", fallback_locale.c_str(), fallback_file_path.c_str());
        return "";
    }
}

std::shared_ptr<IndexedText> loadIndexedText(const std::string& path) {
    std::vector<std::string> data;
    auto file_path = getI18nDataFilePath(path);
    if (file_path.empty()) {
        return nullptr;
    }

    // We lock on folder level, because file is TBD
    file::withLock<void>(path, [&file_path, &data] {
        file::readLines(file_path, true, [&data](const char* line) {
            data.push_back(line);
        });
    });

    if (data.empty()) {
        return nullptr;
    }

    auto result = std::make_shared<IndexedTextImplementation>(data);
    return std::reinterpret_pointer_cast<IndexedText>(result);
}

}
