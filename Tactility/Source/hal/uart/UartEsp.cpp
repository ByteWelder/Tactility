#ifdef ESP_PLATFORM

#include <Tactility/hal/uart/UartEsp.h>

#include <Tactility/Logger.h>
#include <Tactility/kernel/Kernel.h>
#include <Tactility/Mutex.h>

#include <esp_check.h>
#include <sstream>

namespace tt::hal::uart {

static const auto LOGGER = Logger("UART");

bool UartEsp::start() {
    LOGGER.info("[{}] Starting", configuration.name);

    auto lock = mutex.asScopedLock();
    lock.lock();

    if (started) {
        LOGGER.error("[{}] Starting: Already started", configuration.name);
        return false;
    }

    int intr_alloc_flags;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#else
    intr_alloc_flags = 0;
#endif

    esp_err_t result = uart_param_config(configuration.port, &configuration.config);
    if (result != ESP_OK) {
        LOGGER.error("[{}] Starting: Failed to configure: {}", configuration.name, esp_err_to_name(result));
        return false;
    }

    if (uart_is_driver_installed(configuration.port)) {
        LOGGER.error("[{}] Driver was still installed. You probably forgot to stop, or another system uses/used the driver.", configuration.name);
        uart_driver_delete(configuration.port);
    }

    result = uart_set_pin(configuration.port, configuration.txPin, configuration.rxPin, configuration.rtsPin, configuration.ctsPin);
    if (result != ESP_OK) {
        LOGGER.error("[{}] Starting: Failed set pins: {}", configuration.name, esp_err_to_name(result));
        return false;
    }

    result = uart_driver_install(configuration.port, (int)configuration.rxBufferSize, (int)configuration.txBufferSize, 0, nullptr, intr_alloc_flags);
    if (result != ESP_OK) {
        LOGGER.error("[{}] Starting: Failed to install driver: {}", configuration.name, esp_err_to_name(result));
        return false;
    }

    started = true;

    LOGGER.info("[{}] Started", configuration.name);
    return true;
}

bool UartEsp::stop() {
    LOGGER.info("[{}] Stopping", configuration.name);

    auto lock = mutex.asScopedLock();
    lock.lock();

    if (!started) {
        LOGGER.error("[{}] Stopping: Not started", configuration.name);
        return false;
    }

    esp_err_t result = uart_driver_delete(configuration.port);
    if (result != ESP_OK) {
        LOGGER.error("[{}] Stopping: Failed to delete driver: {}", configuration.name, esp_err_to_name(result));
        return false;
    }

    started = false;

    LOGGER.info("[{}] Stopped", configuration.name);
    return true;
}

bool UartEsp::isStarted() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return started;
}

size_t UartEsp::readBytes(std::byte* buffer, size_t bufferSize, TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    auto start_time = kernel::getTicks();
    auto lock_time = kernel::getTicks() - start_time;
    auto remaining_timeout = std::max(timeout - lock_time, 0UL);
    auto result = uart_read_bytes(configuration.port, buffer, bufferSize, remaining_timeout);
    return result;
}

bool UartEsp::readByte(std::byte* output, TickType_t timeout) {
    return readBytes(output, 1, timeout) == 1;
}

size_t UartEsp::writeBytes(const std::byte* buffer, size_t bufferSize, TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    return uart_write_bytes(configuration.port, buffer, bufferSize);
}

size_t UartEsp::available(TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    size_t size = 0;
    uart_get_buffered_data_len(configuration.port, &size);
    return size;
}

void UartEsp::flushInput() {
    uart_flush_input(configuration.port);
}

uint32_t UartEsp::getBaudRate() {
    uint32_t baud_rate = 0;
    auto result = uart_get_baudrate(configuration.port, &baud_rate);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return baud_rate;
}

bool UartEsp::setBaudRate(uint32_t baudRate, TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    auto result = uart_set_baudrate(configuration.port, baudRate);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return result == ESP_OK;
}

std::unique_ptr<Uart> create(const Configuration& configuration) {
    return std::make_unique<UartEsp>(configuration);
}

} // namespace tt::hal::uart

#endif
