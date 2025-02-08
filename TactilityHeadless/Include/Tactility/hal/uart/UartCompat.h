#pragma once

#ifdef ESP_PLATFORM

#include <driver/uart.h>
#include <driver/gpio.h>
#include <hal/uart_types.h>

#else

#include <cstdint>
#define UART_NUM_MAX 3
typedef unsigned int uart_port_t;

typedef struct {
} uart_config_t;

#endif
