#pragma once

#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/uart/Uart.h"

namespace tt::hal::gps {

GpsModel probe(uart::Uart& iart);

}
