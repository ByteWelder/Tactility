#include <Tactility/network/HttpdReq.h>

#include <memory>
#include <ranges>
#include <sstream>
#include <Tactility/Log.h>
#include <Tactility/StringUtils.h>

#ifdef ESP_PLATFORM

namespace tt::network {

constexpr auto* TAG = "HttpdReq";

bool getHeaderOrSendError(httpd_req_t* request, const std::string& name, std::string& value) {
    size_t header_size = httpd_req_get_hdr_value_len(request, name.c_str());
    if (header_size == 0) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "header missing");
        return false;
    }

    auto header_buffer = std::make_unique<char[]>(header_size + 1);
    if (header_buffer == nullptr) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        httpd_resp_send_500(request);
        return false;
    }

    if (httpd_req_get_hdr_value_str(request, name.c_str(), header_buffer.get(), header_size + 1) != ESP_OK) {
        httpd_resp_send_500(request);
        return false;
    }

    value = header_buffer.get();
    return true;
}

bool getMultiPartBoundaryOrSendError(httpd_req_t* request, std::string& boundary) {
    std::string content_type_header;
    if (!getHeaderOrSendError(request, "Content-Type", content_type_header)) {
        return false;
    }

    auto boundary_index = content_type_header.find("boundary=");
    if (boundary_index == std::string::npos) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "boundary not found in Content-Type");
        return false;
    }

    boundary = content_type_header.substr(boundary_index + 9);
    return true;
}

bool getQueryOrSendError(httpd_req_t* request, std::string& query) {
    size_t buffer_length = httpd_req_get_url_query_len(request);
    if (buffer_length == 0) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "id not specified");
        return false;
    }

    auto buffer = std::make_unique<char[]>(buffer_length + 1);
    if (buffer.get() == nullptr || httpd_req_get_url_query_str(request, buffer.get(), buffer_length + 1) != ESP_OK) {
        httpd_resp_send_500(request);
        return false;
    }

    query = buffer.get();

    return true;
}

std::unique_ptr<char[]> receiveByteArray(httpd_req_t* request, size_t length, size_t& bytesRead) {
    assert(length > 0);
    bytesRead = 0;

    // We have to use malloc() because make_unique() throws an exception
    // and we don't have exceptions enabled in the compiler settings
    auto* buffer = static_cast<char*>(malloc(length));
    if (buffer == nullptr) {
        TT_LOG_E(TAG, LOG_MESSAGE_ALLOC_FAILED_FMT, length);
        return nullptr;
    }

    while (bytesRead < length) {
        size_t read_size = length - bytesRead;
        size_t bytes_received = httpd_req_recv(request, buffer + bytesRead, read_size);
        if (bytes_received <= 0) {
            TT_LOG_W(TAG, "Received %zu / %zu", bytesRead + bytes_received, length);
            return nullptr;
        }

        bytesRead += bytes_received;
    }

    return std::unique_ptr<char[]>(std::move(buffer));
}

std::string receiveTextUntil(httpd_req_t* request, const std::string& terminator) {
    size_t read_index = 0;
    std::stringstream result;
    while (!result.str().ends_with(terminator)) {
        char buffer;
        size_t bytes_read = httpd_req_recv(request, &buffer, 1);
        if (bytes_read <= 0) {
            return "";
        } else {
            read_index += bytes_read;
        }

        result << buffer;
    }

    return result.str();
}

std::map<std::string, std::string> parseContentDisposition(const std::vector<std::string>& input) {
    std::map<std::string, std::string> result;
    static std::string prefix = "Content-Disposition: ";

    // Find header
    auto content_disposition_header = std::ranges::find_if(input, [](const std::string& header) {
        return header.starts_with(prefix);
    });

    // Header not found
    if (content_disposition_header == input.end()) {
        return result;
    }

    auto parseable = content_disposition_header->substr(prefix.size());
    auto parts = string::split(parseable, "; ");
    for (auto part : parts) {
        auto key_value = string::split(part, "=");
        if (key_value.size() == 2) {
            // Trim trailing newlines
            auto value = string::trim(key_value[1], "\r\n");
            if (value.size() > 2) {
                result[key_value[0]] = value.substr(1, value.size() - 2);
            } else {
                result[key_value[0]] = "";
            }
        }
    }

    return result;
}

bool readAndDiscardOrSendError(httpd_req_t* request, const std::string& toRead) {
    size_t bytes_read;
    auto buffer = receiveByteArray(request, toRead.length(), bytes_read);
    if (bytes_read != toRead.length()) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "failed to read discardable data");
        return false;
    }

    if (memcmp(buffer.get(), toRead.c_str(), bytes_read) != 0) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "discardable data mismatch");
        return false;
    }

    return true;
}

}

#endif // ESP_PLATFORM