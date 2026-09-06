#pragma once

#include <http/server.h>

#include <string>

// Helper functions for HttpServerRequest from http-module
namespace tt::network {

bool getHeaderOrSendError(struct HttpServerRequest* request, const std::string& name, std::string& value);

bool getMultiPartBoundaryOrSendError(struct HttpServerRequest* request, std::string& boundary);

bool getQueryOrSendError(struct HttpServerRequest* request, std::string& query);

std::string receiveTextUntil(struct HttpServerRequest* request, const std::string& terminator);

bool readAndDiscardOrSendError(struct HttpServerRequest* request, const std::string& toRead);

size_t receiveFile(struct HttpServerRequest* request, size_t length, const std::string& filePath);

}
