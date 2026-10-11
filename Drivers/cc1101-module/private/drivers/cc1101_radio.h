// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/concurrent/recursive_mutex.h>
#include <tactility/concurrent/thread.h>
#include <tactility/drivers/radio.h>
#include <tactility/error.h>
#include <tactility/freertos/event_groups.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

struct Device;
struct GpioDescriptor;

/**
 * Largest packet the driver sends or accepts: the 64-byte FIFO minus the length byte and the
 * two appended status bytes, so a whole packet always fits the FIFO in one go.
 */
constexpr size_t CC1101_MAX_PACKET_LENGTH = 61;

/**
 * CC1101 radio engine: owns the radio thread and the TX queue. The public methods are
 * thread-safe. State/RX/TX events are published via radio_*_event_emit(), either from the radio
 * thread (RX, TX progress, state) or from the caller of transmit() (QUEUED).
 *
 * The RadioLib types live behind the RadioParts indirection: RadioLib declares a global
 * `class Module` that collides with the kernel's `struct Module` when both are visible
 * in the same translation unit, so RadioLib headers must not leak out of the implementation.
 */
class Cc1101Radio final {
public:
    struct Settings {
        /** The kernel device, passed to callbacks */
        Device* device;
        /** The parent SPI controller device, for the SPI controller bus lock */
        Device* spi_controller;
        spi_host_device_t spi_host;
        int spi_frequency_hz;
        /** Native SoC GPIO: the RadioLib HAL drives CS directly through ESP-IDF */
        gpio_num_t pin_cs;
        /** Native SoC GPIO of the GDO0 line, passed to RadioLib */
        gpio_num_t pin_gdo0;
        /** GDO0 IRQ line, owned by the driver. Armed as a HIGH_LEVEL one-shot via the GPIO descriptor callback API. */
        struct GpioDescriptor* gdo0;
        /** Optional band switch lines (nullptr when absent) */
        struct GpioDescriptor* band_switch_0;
        struct GpioDescriptor* band_switch_1;
        uint8_t band_switch_low;
        uint8_t band_switch_mid;
        uint8_t band_switch_high;
    };

private:
    struct RadioParts;

    struct TxItem {
        RadioTxId id = 0;
        std::vector<uint8_t> data;
    };

    const Settings settings;
    RadioParts* parts;
    mutable RecursiveMutex mutex = {};
    EventGroupHandle_t events = nullptr;

    Thread* thread = nullptr;
    bool threadInterrupted = false;

    enum RadioState state = RADIO_STATE_OFF;
    enum RadioModulation modulation = RADIO_MODULATION_NONE;

    std::deque<TxItem> txQueue;
    RadioTxId lastTxId = 0;

    // Parameter store, applied on the next doBegin(). Frequencies/rates are held in base SI
    // units (Hz, bit/s) and converted to RadioLib's MHz/kHz/kbps floats in doBegin().
    // Defaults match RadioLib's CC1101 defaults.
    int8_t power = 10;
    int32_t frequency = 434000000;          // Hz
    int32_t bandwidth = 58000;              // Hz
    int32_t bitRate = 4800;                 // bit/s
    int32_t frequencyDeviation = 5000;      // Hz
    uint16_t preambleLength = 16;           // bits
    uint16_t syncWord = 0x12AD;

    static void gdo0Isr(void* context);
    static int32_t threadMainStatic(void* context);

    void lock() const { recursive_mutex_lock(&mutex); }
    void unlock() const { recursive_mutex_unlock(&mutex); }

    bool isThreadInterrupted() const;
    int32_t threadMain();

    void setState(enum RadioState newState);

    size_t getTxQueueSize() const;
    TxItem popNextQueuedTx();

    void registerGdo0Isr();
    void unregisterGdo0Isr();
    void applyBandSwitch();
    uint32_t getTimeOnAirMillis(size_t length) const;

    int doBegin(enum RadioModulation beginModulation);
    void doEnd();
    void doTransmit();
    bool doListen();
    void doReceive();

public:
    explicit Cc1101Radio(const Settings& settings);
    ~Cc1101Radio();

    /**
     * Verify a live CC1101 responds on the bus by reading its VERSION register.
     * @return ERROR_NONE when the chip responded
     */
    error_t probe();

    enum RadioState getState() const;
    error_t setEnabled(bool enabled);
    error_t setModulation(enum RadioModulation newModulation);
    enum RadioModulation getModulation() const;

    bool canTransmit(enum RadioModulation withModulation) const {
        return (withModulation == RADIO_MODULATION_FSK) ||
               (withModulation == RADIO_MODULATION_FSK4) ||
               (withModulation == RADIO_MODULATION_OOK);
    }

    bool canReceive(enum RadioModulation withModulation) const {
        return canTransmit(withModulation);
    }

    error_t setParameter(enum RadioParameter parameter, int32_t value);
    error_t getParameter(enum RadioParameter parameter, int32_t* value) const;

    error_t transmit(const uint8_t* data, size_t length, RadioTxId* id);
};
