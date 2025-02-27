#ifndef ESP_PLATFORM

#include "Tactility/hal/uart/UartPosix.h"
#include "Tactility/hal/uart/Uart.h"

#include <Tactility/Log.h>

#include <cstring>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#define TAG "uart"

namespace tt::hal::uart {

bool UartPosix::start() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (device != nullptr) {
        TT_LOG_E(TAG, "[%s] Starting: Already started", configuration.name.c_str());
        return false;
    }

    auto file = fopen(configuration.name.c_str(), "w");
    if (file == nullptr) {
        TT_LOG_E(TAG, "[%s] Open device failed", configuration.name.c_str());
        return false;
    }

    auto new_device = std::unique_ptr<FILE, AutoCloseFileDeleter>(file);

    struct termios tty;
    if (tcgetattr(fileno(file), &tty) < 0) {
        printf("[%s] tcgetattr failed: %s\n", configuration.name.c_str(), strerror(errno));
        return false;
    }

    if (cfsetospeed(&tty, (speed_t)configuration.baudRate) == -1) {
        TT_LOG_E(TAG, "[%s] Setting output speed failed", configuration.name.c_str());
    }

    if (cfsetispeed(&tty, (speed_t)configuration.baudRate) == -1) {
        TT_LOG_E(TAG, "[%s] Setting input speed failed", configuration.name.c_str());
    }

    tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; /* 8-bit characters */
    tty.c_cflag &= ~PARENB; /* no parity bit */
    tty.c_cflag &= ~CSTOPB; /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fileno(file), TCSANOW, &tty) != 0) {
        printf("[%s] tcsetattr failed: %s\n", configuration.name.c_str(), strerror(errno));
        return false;
    }

    device = std::move(new_device);

    TT_LOG_I(TAG, "[%s] Started", configuration.name.c_str());
    return true;
}

bool UartPosix::stop() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (device == nullptr) {
        TT_LOG_E(TAG, "[%s] Stopping: Not started", configuration.name.c_str());
        return false;
    }

    device = nullptr;

    TT_LOG_I(TAG, "[%s] Stopped", configuration.name.c_str());
    return true;
}

bool UartPosix::isStarted() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return device != nullptr;
}

size_t UartPosix::readBytes(std::byte* buffer, size_t bufferSize, TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    if (awaitAvailable(timeout)) {
        return read(fileno(device.get()), buffer, bufferSize);
    } else {
        return 0;
    }
}

bool UartPosix::readByte(std::byte* output, TickType_t timeout) {
    if (awaitAvailable(timeout)) {
        return read(fileno(device.get()), output, 1) == 1;
    } else {
        return false;
    }
}

size_t UartPosix::writeBytes(const std::byte* buffer, size_t bufferSize, TickType_t timeout) {
    if (!mutex.lock(timeout)) {
        return false;
    }

    return write(fileno(device.get()), buffer, bufferSize);
}

size_t UartPosix::available(TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    uint32_t bytes_available = 0;
    ioctl(fileno(device.get()), FIONREAD, bytes_available);
    return bytes_available;
}

void UartPosix::flushInput() {
    // TODO
}

uint32_t UartPosix::getBaudRate() {
    struct termios tty;
    if (tcgetattr(fileno(device.get()), &tty) < 0) {
        printf("[%s] tcgetattr failed: %s\n", configuration.name.c_str(), strerror(errno));
        return false;
    } else {
        return (uint32_t)cfgetispeed(&tty);
    }
}

bool UartPosix::setBaudRate(uint32_t baudRate, TickType_t timeout) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(timeout)) {
        return false;
    }

    struct termios tty;
    if (tcgetattr(fileno(device.get()), &tty) < 0) {
        printf("[%s] tcgetattr failed: %s\n", configuration.name.c_str(), strerror(errno));
        return false;
    }

    if (cfsetospeed(&tty, (speed_t)configuration.baudRate) == -1) {
        TT_LOG_E(TAG, "[%s] Failed to set output speed", configuration.name.c_str());
        return false;
    }

    if (cfsetispeed(&tty, (speed_t)configuration.baudRate) == -1) {
        TT_LOG_E(TAG, "[%s] Failed to set input speed", configuration.name.c_str());
        return false;
    }

    return true;
}

bool UartPosix::awaitAvailable(TickType_t timeout) {
    auto start_time = kernel::getTicks();
    do {
        if (available(timeout) > 0) {
            return true;
        }
        kernel::delayTicks(timeout / 10);
    } while ((kernel::getTicks() - start()) < timeout);
    return false;
}

std::unique_ptr<Uart> create(const Configuration& configuration) {
    return std::make_unique<UartPosix>(configuration);
}

} // namespace tt::hal::uart

#endif
