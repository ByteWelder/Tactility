#pragma once
#ifndef ESP_PLATFORM

#include "Tactility/Mutex.h"
#include "Tactility/hal/uart/Configuration.h"
#include "Tactility/hal/uart/Uart.h"

#include <termios.h>

namespace tt::hal::uart {

class UartPosix final : public Uart {

private:
    struct AutoCloseFileDeleter {
        void operator()(FILE* file) {
            fclose(file);
        }
    };

    Mutex mutex;
    const Configuration& configuration;
    std::unique_ptr<FILE, AutoCloseFileDeleter> device;

    bool awaitAvailable(TickType_t timeout);

public:

    explicit UartPosix(const Configuration& configuration) : configuration(configuration) {}

    bool start() final;
    bool isStarted() const final;
    bool stop() final;
    size_t readBytes(std::byte* buffer, size_t bufferSize, TickType_t timeout) final;
    bool readByte(std::byte* output, TickType_t timeout) final;
    size_t writeBytes(const std::byte* buffer, size_t bufferSize, TickType_t timeout) final;
    size_t available(TickType_t timeout) final;
    bool setBaudRate(uint32_t baudRate, TickType_t timeout) final;
    uint32_t getBaudRate() final;
    void flushInput() final;
};

std::unique_ptr<Uart> create(const Configuration& configuration);

} // namespace tt::hal::uart

#endif