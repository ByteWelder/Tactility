#pragma once

#include <Tactility/hal/uart/Uart.h>

namespace tt::hal::gps {

struct GpsInfo;

bool initGpsM10Q(uart_port_t port, GpsInfo& info);

}
