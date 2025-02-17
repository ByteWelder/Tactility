#pragma once

#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/uart/Uart.h"

namespace tt::hal::gps {

struct GpsInfo;

GpsModel probe(uart_port_t port);

}
