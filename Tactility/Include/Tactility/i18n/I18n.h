#pragma once

#include <string>
#include <memory>

namespace tt::i18n {

class IndexedText {

public:

    virtual ~IndexedText() = default;

    virtual const std::string& get(int index) const = 0;

    template <typename EnumType>
    const std::string& get(EnumType value) const { return get(static_cast<int>(value)); }

    const std::string& operator[](const int index) const { return get(index); }
};

std::shared_ptr<IndexedText> loadIndexedText(const std::string& path);

}