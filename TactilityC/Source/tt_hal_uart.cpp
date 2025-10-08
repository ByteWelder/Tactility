#include "tt_hal_uart.h"
#include <Tactility/hal/uart/Uart.h>

using namespace tt::hal;

struct UartWrapper {
    std::shared_ptr<uart::Uart> uart;
};

#define HANDLE_AS_UART(handle) static_cast<UartWrapper*>(handle)->uart

extern "C" {

size_t tt_hal_uart_get_count() {
    return uart::getNames().size();
}

bool tt_hal_uart_get_name(size_t index, char* name, size_t nameSizeLimit) {
    assert(index < uart::getNames().size());
    auto source_name = uart::getNames()[index];
    return strncpy(name, source_name.c_str(), nameSizeLimit) != nullptr;
}

UartHandle tt_hal_uart_alloc(size_t index) {
    assert(index < uart::getNames().size());
    auto* wrapper = new UartWrapper();
    auto name = uart::getNames()[index];
    wrapper->uart = uart::open(name);
    assert(wrapper->uart != nullptr);
    return wrapper;
}

void tt_hal_uart_free(UartHandle handle) {
    auto* wrapper = static_cast<UartWrapper*>(handle);
    assert(wrapper->uart != nullptr);
    if (wrapper->uart->isStarted()) {
        wrapper->uart->stop();
    }
    delete wrapper;
}

bool tt_hal_uart_start(UartHandle handle) {
    return HANDLE_AS_UART(handle)->start();
}

bool tt_hal_uart_is_started(UartHandle handle) {
    return HANDLE_AS_UART(handle)->isStarted();
}

bool tt_hal_uart_stop(UartHandle handle) {
    return HANDLE_AS_UART(handle)->stop();
}

size_t tt_hal_uart_read_bytes(UartHandle handle, char* buffer, size_t bufferSize, TickType timeout) {
    return HANDLE_AS_UART(handle)->readBytes(reinterpret_cast<std::byte*>(buffer), bufferSize, timeout);
}

bool tt_hal_uart_read_byte(UartHandle handle, char* output, TickType timeout) {
    return HANDLE_AS_UART(handle)->readByte(reinterpret_cast<std::byte*>(output), timeout);
}

size_t tt_hal_uart_write_bytes(UartHandle handle, const char* buffer, size_t bufferSize, TickType timeout) {
    return HANDLE_AS_UART(handle)->writeBytes(reinterpret_cast<const std::byte*>(buffer), bufferSize, timeout);
}

size_t tt_hal_uart_available(UartHandle handle) {
    return HANDLE_AS_UART(handle)->available();
}

bool tt_hal_uart_set_baud_rate(UartHandle handle, size_t baud_rate) {
    return HANDLE_AS_UART(handle)->setBaudRate(baud_rate);
}

uint32_t tt_hal_uart_get_baud_rate(UartHandle handle) {
    return HANDLE_AS_UART(handle)->getBaudRate();
}

void tt_hal_uart_flush_input(UartHandle handle) {
    HANDLE_AS_UART(handle)->flushInput();
}

}
