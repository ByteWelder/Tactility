#pragma once

#include "Tactility/hal/gps/GpsDevice.h"
#include "GpsState.h"

#include <Tactility/PubSub.h>

namespace tt::service::gps {

/** Register a hardware device to the GPS service. */
void addGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);

/** Deregister a hardware device to the GPS service. */
void removeGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);

/** @return true when GPS is set to receive updates from at least 1 device */
bool startReceiving();

/** Turn GPS receiving off */
void stopReceiving();

bool hasCoordinates();

bool getCoordinates(minmea_sentence_rmc& rmc);

State getState();

/** @return GPS service pubsub that broadcasts State* objects */
std::shared_ptr<PubSub> getStatePubsub();

}
