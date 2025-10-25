#pragma once

#include <string>
#include <functional>

namespace tt::network::http {
    /**
     * Download a file from a URL.
     * The server must send the Content-Length header.
     * @param url download source URL
     * @param certFilePath the path to the .pem file
     * @param downloadFilePath The path to downloadd the file to. The parent directories must exist.
     * @param onSuccess the success result callback
     * @param onError the error result callback
     */
    void download(
    const std::string& url,
    const std::string& certFilePath,
    const std::string &downloadFilePath,
    std::function<void()> onSuccess,
    std::function<void(const char* errorMessage)> onError
);

}
