#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <tt_kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* UartHandle;

size_t tt_hal_uart_get_count();

bool tt_hal_uart_get_name(size_t index, char* name, size_t nameSizeLimit);

UartHandle tt_hal_uart_alloc(size_t index);

void tt_hal_uart_free(UartHandle handle);

bool tt_hal_uart_start(UartHandle handle);

bool tt_hal_uart_is_started(UartHandle handle);

bool tt_hal_uart_stop(UartHandle handle);

size_t tt_hal_uart_read_bytes(UartHandle handle, char* buffer, size_t bufferSize, TickType timeout);

bool tt_hal_uart_read_byte(UartHandle handle, char* output, TickType timeout);

size_t tt_hal_uart_write_bytes(UartHandle handle, const char* buffer, size_t bufferSize, TickType timeout);

size_t tt_hal_uart_available(UartHandle handle);

bool tt_hal_uart_set_baud_rate(UartHandle handle, size_t baud_rate);

uint32_t tt_hal_uart_get_baud_rate(UartHandle handle);

void tt_hal_uart_flush_input(UartHandle handle);

#ifdef __cplusplus
}
#endif
