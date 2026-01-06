#include "Bq25896.h"

#include <Tactility/Logger.h>

static const auto LOGGER = tt::Logger("BQ25896");

void Bq25896::powerOff() {
    LOGGER.info("Power off");
    bitOn(0x09, BIT(5));
}

void Bq25896::powerOn() {
    LOGGER.info("Power on");
    bitOff(0x09, BIT(5));
}
