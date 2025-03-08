#include "Tactility/hal/uart/Uart.h"

#include <Tactility/Log.h>
#include <Tactility/Mutex.h>

#include <ranges>
#include <cstring>

#ifdef ESP_PLATFORM
#include "Tactility/TactilityHeadless.h"
#include "Tactility/hal/uart/UartEsp.h"
#include <esp_check.h>
#else
#include "Tactility/hal/uart/UartPosix.h"
#include <dirent.h>
#endif

#define TAG "uart"

namespace tt::hal::uart {

constexpr uint32_t uartIdNotInUse = 0;

struct UartEntry {
    uint32_t usageId = uartIdNotInUse;
    Configuration configuration;
};

static std::vector<UartEntry> uartEntries = {};
static uint32_t lastUartId = uartIdNotInUse;

bool init(const std::vector<uart::Configuration>& configurations) {
    TT_LOG_I(TAG, "Init");
    for (const auto& configuration: configurations) {
        uartEntries.push_back({
            .usageId = uartIdNotInUse,
            .configuration = configuration
        });
    }

    return true;
}

bool Uart::writeString(const char* buffer, TickType_t timeout) {
    auto size = strlen(buffer);
    writeBytes((std::byte*)buffer, size, timeout);
    return true;
}

size_t Uart::readUntil(std::byte* buffer, size_t bufferSize, uint8_t untilByte, TickType_t timeout, bool addNullTerminator) {
    TickType_t start_time = kernel::getTicks();
    auto* buffer_write_ptr = reinterpret_cast<uint8_t*>(buffer);
    uint8_t* buffer_limit = buffer_write_ptr + bufferSize - 1; // Keep 1 extra char as mull terminator
    TickType_t timeout_left = timeout;
    while (readByte(reinterpret_cast<std::byte*>(buffer_write_ptr), timeout_left) && buffer_write_ptr < buffer_limit) {
#ifdef DEBUG_READ_UNTIL
        // If first successful read and we're not receiving an empty response
        if (buffer_write_ptr == buffer && *buffer_write_ptr != 0x00U && *buffer_write_ptr != untilByte) {
            printf(">>");
        }
#endif

        if (*buffer_write_ptr == untilByte) {
            // TODO: Fix when untilByte is null terminator char already
            if (addNullTerminator) {
                buffer_write_ptr++;
                *buffer_write_ptr = 0x00U;
            }
            break;
        }

#ifdef DEBUG_READ_UNTIL
        printf("%c", *buffer_write_ptr);
#endif

        buffer_write_ptr++;

        TickType_t now = kernel::getTicks();
        if (now > (start_time + timeout)) {
#ifdef DEBUG_READ_UNTIL
            TT_LOG_W(TAG, "readUntil() timeout");
#endif
            break;
        } else {
            timeout_left = timeout - (now - start_time);
        }
    }

#ifdef DEBUG_READ_UNTIL
    // If we read data and it's not an empty response
    if (buffer_write_ptr != buffer && *buffer != 0x00U && *buffer != untilByte) {
        printf("\n");
    }
#endif

    if (addNullTerminator && (buffer_write_ptr > reinterpret_cast<uint8_t*>(buffer))) {
        return reinterpret_cast<size_t>(buffer_write_ptr) - reinterpret_cast<size_t>(buffer) - 1UL;
    } else {
        return reinterpret_cast<size_t>(buffer_write_ptr) - reinterpret_cast<size_t>(buffer);
    }
}

std::unique_ptr<Uart> open(std::string name) {
    TT_LOG_I(TAG, "Open %s", name.c_str());

    auto result = std::views::filter(uartEntries, [&name](auto& entry) {
        return entry.configuration.name == name;
    });

    if (result.empty()) {
        TT_LOG_E(TAG, "UART not found: %s", name.c_str());
        return nullptr;
    }

    auto& entry = *result.begin();
    if (entry.usageId != uartIdNotInUse) {
        TT_LOG_E(TAG, "UART in use: %s", name.c_str());
        return nullptr;
    }

    auto uart = create(entry.configuration);
    assert(uart != nullptr);
    entry.usageId = uart->getId();
    TT_LOG_I(TAG, "Opened %lu", entry.usageId);
    return uart;
}

void close(uint32_t uartId) {
    TT_LOG_I(TAG, "Close %lu", uartId);
    auto result = std::views::filter(uartEntries, [&uartId](auto& entry) {
      return entry.usageId == uartId;
    });

    if (!result.empty()) {
        auto& entry = *result.begin();
        entry.usageId = uartIdNotInUse;
    } else {
        TT_LOG_W(TAG, "Auto-closing UART, but can't find it");
    }
}

std::vector<std::string> getNames() {
    std::vector<std::string> names;
#ifdef ESP_PLATFORM
    for (auto& config : getConfiguration()->uart) {
        names.push_back(config.name);
    }
#else
    DIR* dir = opendir("/dev");
    if (dir == nullptr) {
        TT_LOG_E(TAG, "Failed to read /dev");
        return names;
    }
    struct dirent* current_entry;
    while ((current_entry = readdir(dir)) != nullptr) {
        auto name = std::string(current_entry->d_name);
        if (name.starts_with("tty")) {
            auto path = std::string("/dev/") + name;
            names.push_back(path);
        }
    }

    closedir(dir);
#endif
    return names;
}

Uart::Uart() : id(++lastUartId) {}

Uart::~Uart() {
    close(getId());
}

} // namespace tt::hal::uart
