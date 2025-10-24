#pragma once

#include <string>

namespace tt::network::http {

void download(
    const std::string& url,
    const std::string& certFilePath,
    const std::string &downloadFilePath,
    std::function<void()> onSuccess,
    std::function<void(const char* errorMessage)> onError
);

}
