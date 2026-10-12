#include "doctest.h"

#include <location/gps_provider.h>

#include <cmath>

namespace {

constexpr auto* RMC = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
constexpr auto* GGA = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

}

TEST_CASE("gps_location_from_nmea converts RMC and GGA") {
    minmea_sentence_rmc rmc;
    minmea_sentence_gga gga;
    REQUIRE(minmea_parse_rmc(&rmc, RMC));
    REQUIRE(minmea_parse_gga(&gga, GGA));

    Location location;
    REQUIRE(gps_location_from_nmea(&rmc, &gga, &location));
    CHECK(location.latitude == doctest::Approx(48.1173));
    CHECK(location.longitude == doctest::Approx(11.516667));
    CHECK(location.speed == doctest::Approx(11.5235).epsilon(0.001));
    CHECK(location.course == doctest::Approx(84.4));
    CHECK(location.horizontal_accuracy == doctest::Approx(4.5));
    CHECK(location.altitude == doctest::Approx(545.4));
    CHECK(location.ellipsoidal_altitude == doctest::Approx(592.3));
    CHECK(std::isnan(location.vertical_accuracy));
    CHECK(location.satellites == 8);
    CHECK(location.time_ms == 764426119000LL);
}

TEST_CASE("gps_location_from_nmea without GGA has no altitude or accuracy") {
    minmea_sentence_rmc rmc;
    REQUIRE(minmea_parse_rmc(&rmc, RMC));

    Location location;
    REQUIRE(gps_location_from_nmea(&rmc, nullptr, &location));
    CHECK(location.latitude == doctest::Approx(48.1173));
    CHECK(std::isnan(location.altitude));
    CHECK(std::isnan(location.horizontal_accuracy));
    CHECK(location.satellites == 0);
}

TEST_CASE("gps_location_from_nmea handles the southern and western hemispheres") {
    minmea_sentence_rmc rmc;
    REQUIRE(minmea_parse_rmc(&rmc, "$GPRMC,123519,A,3352.128,S,15112.558,W,000.0,,230394,,*00"));

    Location location;
    REQUIRE(gps_location_from_nmea(&rmc, nullptr, &location));
    CHECK(location.latitude == doctest::Approx(-33.8688));
    CHECK(location.longitude == doctest::Approx(-151.209300));
    CHECK(location.speed == doctest::Approx(0.0));
    CHECK(std::isnan(location.course));
}

TEST_CASE("gps_location_from_nmea rejects a fix that is not valid") {
    minmea_sentence_rmc rmc;
    REQUIRE(minmea_parse_rmc(&rmc, "$GPRMC,123519,V,,,,,,,230394,,*00"));

    Location location;
    CHECK_FALSE(gps_location_from_nmea(&rmc, nullptr, &location));
}
