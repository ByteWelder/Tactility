#pragma once

#include "Tactility/hal/gps/GpsDevice.h"

namespace tt::service::gps {

/** Register a hardware device to the GPS service. */
void addGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);

/** Deregister a hardware device to the GPS service. */
void removeGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);

/** @return true when GPS is set to receive updates from at least 1 device */
bool startReceiving();

/** Turn GPS receiving off */
void stopReceiving();

/** @return true when GPS receiver is on and 1 or more devices are active */
bool isReceiving();

}
