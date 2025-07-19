#pragma once

#ifdef ESP_PLATFORM

#include <esp_http_server.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tt::network {

bool getHeaderOrSendError(httpd_req_t* request, const std::string& name, std::string& value);

bool getMultiPartBoundaryOrSendError(httpd_req_t* request, std::string& boundary);

bool getQueryOrSendError(httpd_req_t* request, std::string& query);

std::unique_ptr<char[]> receiveByteArray(httpd_req_t* request, size_t length, size_t& bytesRead);

std::string receiveTextUntil(httpd_req_t* request, const std::string& terminator);

std::map<std::string, std::string> parseContentDisposition(const std::vector<std::string>& input);

bool readAndDiscardOrSendError(httpd_req_t* request, const std::string& toRead);

}

#endif // ESP_PLATFORM