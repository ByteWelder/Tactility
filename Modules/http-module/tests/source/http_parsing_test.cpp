#include "doctest.h"

#include <http/private/http_parsing.h>

TEST_CASE("http_parse_https_url splits the host, port and path") {
    HttpUrl url;
    REQUIRE(http_parse_https_url("https://cdn.example.org/apps/1.0/apps.json?x=1#top", url));
    CHECK_EQ(url.host, "cdn.example.org");
    CHECK_EQ(url.port, "443");
    CHECK_EQ(url.path, "/apps/1.0/apps.json?x=1");

    REQUIRE(http_parse_https_url("https://example.org:8443", url));
    CHECK_EQ(url.port, "8443");
    CHECK_EQ(url.path, "/");

    REQUIRE(http_parse_https_url("https://[::1]:9000/file", url));
    CHECK_EQ(url.host, "::1");
    CHECK_EQ(url.port, "9000");
}

TEST_CASE("http_parse_https_url rejects other schemes and malformed URLs") {
    HttpUrl url;
    CHECK_FALSE(http_parse_https_url("http://example.org/", url));
    CHECK_FALSE(http_parse_https_url("https://", url));
    CHECK_FALSE(http_parse_https_url("https://example.org:abc/", url));
    CHECK_FALSE(http_parse_https_url("https://user@example.org/", url));
}

TEST_CASE("http_parse_status_line reads the status code") {
    int status = 0;
    REQUIRE(http_parse_status_line("HTTP/1.1 200 OK", status));
    CHECK_EQ(status, 200);
    REQUIRE(http_parse_status_line("HTTP/1.1 404", status));
    CHECK_EQ(status, 404);
    CHECK_FALSE(http_parse_status_line("HTTP/1.1 20 OK", status));
    CHECK_FALSE(http_parse_status_line("SSH-2.0 200", status));
}

TEST_CASE("http_parse_header_line lowercases the name and trims the value") {
    std::string name;
    std::string value;
    REQUIRE(http_parse_header_line("Content-Length:  42 ", name, value));
    CHECK_EQ(name, "content-length");
    CHECK_EQ(value, "42");
    CHECK_FALSE(http_parse_header_line("no colon here", name, value));
    CHECK_FALSE(http_parse_header_line(": empty name", name, value));
}

TEST_CASE("http_parse_chunk_size reads hexadecimal sizes and ignores extensions") {
    size_t size = 0;
    REQUIRE(http_parse_chunk_size("1a2B", size));
    CHECK_EQ(size, 0x1a2b);
    REQUIRE(http_parse_chunk_size("0", size));
    CHECK_EQ(size, 0);
    REQUIRE(http_parse_chunk_size("ff;name=value", size));
    CHECK_EQ(size, 0xff);
    CHECK_FALSE(http_parse_chunk_size("", size));
    CHECK_FALSE(http_parse_chunk_size("xyz", size));
}
