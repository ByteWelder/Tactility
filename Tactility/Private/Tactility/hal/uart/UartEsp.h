#pragma once
#ifdef ESP_PLATFORM

#include "Tactility/Mutex.h"
#include "Tactility/hal/uart/Uart.h"
#include "Tactility/hal/uart/Configuration.h"

namespace tt::hal::uart {

class UartEsp final : public Uart {

    Mutex mutex;
    const Configuration& configuration;
    bool started = false;

public:

    explicit UartEsp(const Configuration& configuration) : configuration(configuration) {}

    bool start() override;
    bool isStarted() const override;
    bool stop() override;
    size_t readBytes(std::byte* buffer, size_t bufferSize, TickType_t timeout) override;
    bool readByte(std::byte* output, TickType_t timeout) override;
    size_t writeBytes(const std::byte* buffer, size_t bufferSize, TickType_t timeout) override;
    size_t available(TickType_t timeout) override;
    bool setBaudRate(uint32_t baudRate, TickType_t timeout) override;
    uint32_t getBaudRate() override;
    void flushInput() override;
};

std::unique_ptr<Uart> create(const Configuration& configuration);

} // namespace tt::hal::uart

#endif