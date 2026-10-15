#pragma once

#include <cstddef>
#include <string>

namespace tt::app::development {

constexpr size_t FINGERPRINT_WORD_LENGTH = 4;

/** Groups a fingerprint in words of 4 characters: "xxxx xxxx xxxx ..." */
inline std::string formatFingerprint(const std::string& fingerprint) {
    std::string result;
    for (size_t i = 0; i < fingerprint.size(); i += FINGERPRINT_WORD_LENGTH) {
        if (i > 0) {
            result += ' ';
        }
        result += fingerprint.substr(i, FINGERPRINT_WORD_LENGTH);
    }
    return result;
}

/** Shows the first 2 words of a fingerprint: "xxxx xxxx ..." */
inline std::string formatShortFingerprint(const std::string& fingerprint) {
    if (fingerprint.size() <= FINGERPRINT_WORD_LENGTH * 2) {
        return formatFingerprint(fingerprint);
    }
    return formatFingerprint(fingerprint.substr(0, FINGERPRINT_WORD_LENGTH * 2)) + " ...";
}

} // namespace tt::app::development
