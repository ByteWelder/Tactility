#include "Tactility/i18n/I18n.h"
#include "Tactility/file/FileLock.h"

#include <cstring>
#include <vector>

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
    return "nl-NL";
}

static std::string getFallbackLocale() {
    // TODO: Implement locale settings
    return "en-GB";
}

static FILE* openI18nFile(const std::string& path) {
    auto locale = getDesiredLocale();
    auto desired_file_path = std::format("{}/{}.i18n", path, locale);
    auto* file = fopen(desired_file_path.c_str(), "r");
    if (file == nullptr) {
        auto fallback_locale = getFallbackLocale();
        TT_LOG_W(TAG, "Translations not found for %s at %s", locale.c_str(), desired_file_path.c_str());
        auto fallback_file_path = std::format("{}/{}.i18n", path, getFallbackLocale());
        file = fopen(fallback_file_path.c_str(), "r");
        if (file == nullptr) {
            TT_LOG_W(TAG, "Fallback translations not found for %s at %s", fallback_locale.c_str(), fallback_file_path.c_str());
        }
    }
    return file;
}

std::shared_ptr<IndexedText> loadIndexedText(const std::string& path) {
    std::vector<std::string> data;

    // We lock on folder level, because file is TBD
    file::withLock<void>(path, [&path, &data] {
        auto* file = openI18nFile(path);
        if (file != nullptr) {
            char line[1024];
            // TODO: move to file::readLines(filePath, skipEndline, callback)
            while (fgets(line, sizeof(line), file) != nullptr) {
                // Strip newline
                size_t line_length = strlen(line);
                if (line_length > 0 && line[line_length - 1] == '\n') {
                    line[line_length - 1] = '\0';
                }
                // Publish
                data.push_back(line);
            }
            fclose(file);
        }
    });

    if (data.empty()) {
        return nullptr;
    }

    auto result = std::make_shared<IndexedTextImplementation>(data);
    return std::reinterpret_pointer_cast<IndexedText>(result);
}

}
