// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <string>

struct HttpUrl {
    std::string host;
    std::string port;
    /** The path and query, "/" when the URL has none */
    std::string path;
};

/** Parses an https:// URL. The port is 443 when the URL has none. */
bool http_parse_https_url(const std::string& url, HttpUrl& out);

/** Parses a status line such as "HTTP/1.1 200 OK" */
bool http_parse_status_line(const std::string& line, int& statusCode);

/** Parses a header line such as "Content-Length: 42". The name is returned in lower case, the value without surrounding whitespace. */
bool http_parse_header_line(const std::string& line, std::string& name, std::string& value);

/** Parses the size line of a chunk in a chunked body, e.g. "1a2b" or "1a2b;name=value" */
bool http_parse_chunk_size(const std::string& line, size_t& size);
