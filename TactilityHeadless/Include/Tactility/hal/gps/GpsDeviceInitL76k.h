#pragma once

#include <Tactility/hal/uart/Uart.h>

namespace tt::hal::gps {

bool initGpsL76k(uart_port_t port);

}
