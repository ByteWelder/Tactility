#include <Tactility/network/HttpServerReq.h>

#include <tactility/log.h>

#include <algorithm>

namespace tt::network {

constexpr auto* TAG = "HttpServerReq";

bool getHeaderOrSendError(struct HttpServerRequest* request, const std::string& name, std::string& value) {
    size_t length = http_server_request_get_header(request, name.c_str(), nullptr, 0);
    if (length == 0) {
        http_server_request_send_error(request, 400, "header missing");
        return false;
    }
    value.resize(length);
    http_server_request_get_header(request, name.c_str(), value.data(), length + 1);
    return true;
}

bool getMultiPartBoundaryOrSendError(struct HttpServerRequest* request, std::string& boundary) {
    std::string content_type;
    if (!getHeaderOrSendError(request, "Content-Type", content_type)) {
        return false;
    }

    auto boundary_index = content_type.find("boundary=");
    if (boundary_index == std::string::npos) {
        http_server_request_send_error(request, 400, "boundary not found in Content-Type");
        return false;
    }

    boundary = content_type.substr(boundary_index + 9);
    boundary = boundary.substr(0, boundary.find(';'));
    // Trim any whitespace left by the ';' cut above, then unquote (RFC 2231 allows a quoted value).
    while (!boundary.empty() && boundary.back() == ' ') {
        boundary.pop_back();
    }
    if (boundary.size() >= 2 && boundary.front() == '"' && boundary.back() == '"') {
        boundary = boundary.substr(1, boundary.size() - 2);
    }
    return true;
}

bool getQueryOrSendError(struct HttpServerRequest* request, std::string& query) {
    size_t length = http_server_request_get_query(request, nullptr, 0);
    if (length == 0) {
        http_server_request_send_error(request, 400, "id not specified");
        return false;
    }
    query.resize(length);
    http_server_request_get_query(request, query.data(), length + 1);
    return true;
}

// Reads exactly `length` bytes, or fails - unlike http_server_request_receive() itself, which may
// return fewer bytes than requested per call.
static bool receiveExact(struct HttpServerRequest* request, void* buffer, size_t length) {
    size_t total_read = 0;
    while (total_read < length) {
        int read = http_server_request_receive(request, static_cast<char*>(buffer) + total_read, length - total_read);
        if (read <= 0) {
            return false;
        }
        total_read += static_cast<size_t>(read);
    }
    return true;
}

// Bounds a client's multipart preamble (boundary + part headers): without this, a client that
// keeps sending bytes without the terminator would make this buffer, and re-scan, unboundedly.
constexpr size_t MAX_PREAMBLE_LENGTH = 8192;

std::string receiveTextUntil(struct HttpServerRequest* request, const std::string& terminator) {
    std::string result;
    while (!result.ends_with(terminator)) {
        if (result.length() >= MAX_PREAMBLE_LENGTH) {
            return "";
        }
        char byte;
        if (!receiveExact(request, &byte, 1)) {
            return "";
        }
        result += byte;
    }
    return result;
}

bool readAndDiscardOrSendError(struct HttpServerRequest* request, const std::string& toRead) {
    std::string buffer(toRead.length(), '\0');
    if (!receiveExact(request, buffer.data(), toRead.length())) {
        http_server_request_send_error(request, 400, "failed to read discardable data");
        return false;
    }
    if (buffer != toRead) {
        http_server_request_send_error(request, 400, "discardable data mismatch");
        return false;
    }
    return true;
}

size_t receiveFile(struct HttpServerRequest* request, size_t length, const std::string& filePath) {
    constexpr size_t BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    size_t bytes_received = 0;

    auto* file = fopen(filePath.c_str(), "wb");
    if (file == nullptr) {
        LOG_E(TAG, "Failed to open file for writing: %s", filePath.c_str());
        return 0;
    }

    while (bytes_received < length) {
        size_t expected_chunk_size = std::min<size_t>(BUFFER_SIZE, length - bytes_received);
        int received = http_server_request_receive(request, buffer, expected_chunk_size);
        if (received <= 0) {
            LOG_E(TAG, "Receive failed, got 0 bytes but expected %zu more", length - bytes_received);
            break;
        }
        if (fwrite(buffer, 1, static_cast<size_t>(received), file) != static_cast<size_t>(received)) {
            LOG_E(TAG, "Failed to write all bytes");
            break;
        }
        bytes_received += static_cast<size_t>(received);
    }

    fclose(file);
    return bytes_received;
}

}
