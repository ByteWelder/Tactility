#include "doctest.h"

#include <location/ip_provider.h>

#include <cstring>

TEST_CASE("ip_api_parse_response parses a successful lookup") {
    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "X-Ttl: 60\r\n"
        "X-Rl: 44\r\n"
        "\r\n"
        "success\n"
        "39.03\n"
        "-77.5\n";
    IpApiResponse parsed;
    REQUIRE(ip_api_parse_response(response, &parsed));
    CHECK(parsed.status_code == 200);
    CHECK(parsed.success);
    CHECK(parsed.latitude == doctest::Approx(39.03));
    CHECK(parsed.longitude == doctest::Approx(-77.5));
    CHECK(parsed.rate_remaining == 44);
    CHECK(parsed.rate_reset_seconds == 60);
}

TEST_CASE("ip_api_parse_response parses a failed lookup") {
    const char* response = "HTTP/1.1 200 OK\r\nx-rl: 0\r\nx-ttl: 12\r\n\r\nfail\nprivate range\n";
    IpApiResponse parsed;
    REQUIRE(ip_api_parse_response(response, &parsed));
    CHECK_FALSE(parsed.success);
    CHECK(strcmp(parsed.message, "private range") == 0);
    CHECK(parsed.rate_remaining == 0);
    CHECK(parsed.rate_reset_seconds == 12);
}

TEST_CASE("ip_api_parse_response handles a throttled request") {
    const char* response = "HTTP/1.1 429 Too Many Requests\r\nX-Rl: 0\r\nX-Ttl: 30\r\n\r\n";
    IpApiResponse parsed;
    REQUIRE(ip_api_parse_response(response, &parsed));
    CHECK(parsed.status_code == 429);
    CHECK_FALSE(parsed.success);
    CHECK(parsed.rate_reset_seconds == 30);
}

TEST_CASE("ip_api_parse_response rejects malformed responses") {
    IpApiResponse parsed;
    CHECK_FALSE(ip_api_parse_response("", &parsed));
    CHECK_FALSE(ip_api_parse_response("garbage", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\nX-Rl: 1\r\n", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\n\r\nsuccess\nabc\n1.0\n", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\n\r\nunknown\n", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\n\r\nsuccess\nnan\n1.0\n", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\n\r\nsuccess\n1.0\ninf\n", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\n\r\nsuccess\n90.5\n1.0\n", &parsed));
    CHECK_FALSE(ip_api_parse_response("HTTP/1.1 200 OK\r\n\r\nsuccess\n1.0\n-180.5\n", &parsed));
}
