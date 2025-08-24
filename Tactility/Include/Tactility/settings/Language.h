#pragma once

#include <string>

namespace tt::settings {

enum class Language {
    en_GB,
    en_US,
    fr_FR,
    nl_BE,
    nl_NL,
    count
};

void setLanguage(Language language);

Language getLanguage();

std::string toString(Language language);

bool fromString(const std::string& text, Language& language);

}
