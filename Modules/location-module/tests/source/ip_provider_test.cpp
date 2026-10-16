#include "doctest.h"

#include <location/ip_provider.h>

#include <cstring>
#include <string>

TEST_CASE("ip_lookup_parse_response parses a successful lookup") {
    const char* response = R"({"ip":"192.0.2.1","latitude":52.3676,"longitude":4.9041,"city":"Amsterdam","timezone_name":"Europe/Amsterdam","success":true,"premium":false})";
    IpLookupResponse parsed;
    REQUIRE(ip_lookup_parse_response(response, &parsed));
    CHECK(parsed.success);
    CHECK(parsed.latitude == doctest::Approx(52.3676));
    CHECK(parsed.longitude == doctest::Approx(4.9041));
    CHECK(strcmp(parsed.time_zone, "Europe/Amsterdam") == 0);
}

TEST_CASE("ip_lookup_parse_response accepts a lookup without a time zone") {
    IpLookupResponse parsed;
    REQUIRE(ip_lookup_parse_response(R"({"latitude":-33.9,"longitude":151.2,"success":true})", &parsed));
    CHECK(parsed.success);
    CHECK(parsed.time_zone[0] == '\0');
}

TEST_CASE("ip_lookup_parse_response rejects an invalid time zone") {
    IpLookupResponse parsed;
    CHECK_FALSE(ip_lookup_parse_response(R"({"latitude":52.37,"longitude":4.89,"timezone_name":"../etc/passwd x","success":true})", &parsed));
    const std::string tooLong = R"({"latitude":52.37,"longitude":4.89,"timezone_name":")" + std::string(LOCATION_TIME_ZONE_MAX_LENGTH + 1, 'a') + R"(","success":true})";
    CHECK_FALSE(ip_lookup_parse_response(tooLong.c_str(), &parsed));
}

TEST_CASE("ip_lookup_parse_response parses a failed lookup") {
    IpLookupResponse parsed;
    REQUIRE(ip_lookup_parse_response(R"({"success":false})", &parsed));
    CHECK_FALSE(parsed.success);
}

TEST_CASE("ip_lookup_parse_response rejects malformed responses") {
    IpLookupResponse parsed;
    CHECK_FALSE(ip_lookup_parse_response("", &parsed));
    CHECK_FALSE(ip_lookup_parse_response("garbage", &parsed));
    CHECK_FALSE(ip_lookup_parse_response("[]", &parsed));
    CHECK_FALSE(ip_lookup_parse_response(R"({"latitude":52.37,"longitude":4.89})", &parsed));
    CHECK_FALSE(ip_lookup_parse_response(R"({"latitude":"52.37","longitude":4.89,"success":true})", &parsed));
    CHECK_FALSE(ip_lookup_parse_response(R"({"longitude":4.89,"success":true})", &parsed));
    CHECK_FALSE(ip_lookup_parse_response(R"({"latitude":90.5,"longitude":1.0,"success":true})", &parsed));
    CHECK_FALSE(ip_lookup_parse_response(R"({"latitude":1.0,"longitude":-180.5,"success":true})", &parsed));
}
