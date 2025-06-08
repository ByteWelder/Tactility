#include "tt_gps.h"
#include <Tactility/service/gps/GpsService.h>

using namespace tt::service;

extern "C" {

bool tt_gps_has_coordinates() {
    auto service = gps::findGpsService();
    return service != nullptr && service->hasCoordinates();
}

bool tt_gps_get_coordinates(
    float& longitude,
    float& latitude,
    float& speed,
    float& course,
    int& day,
    int& month,
    int& year
) {
    auto service = gps::findGpsService();

    if (service == nullptr) {
        return false;
    }

    minmea_sentence_rmc rmc;

    if (!service->getCoordinates(rmc)) {
        return false;
    }

    longitude = minmea_tocoord(&rmc.longitude);
    latitude = minmea_tocoord(&rmc.latitude);
    speed = minmea_tocoord(&rmc.speed);
    course = minmea_tocoord(&rmc.course);
    day = rmc.date.day;
    month = rmc.date.month;
    year = rmc.date.year;

    return true;
}

}