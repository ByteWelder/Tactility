// SPDX-License-Identifier: Apache-2.0
#include "cc1101_radio.h"

#include "cc1101_radiolib_hal.h"

#include <tactility/concurrent/event_group.h>
#include <tactility/delay.h>
#include <tactility/drivers/gpio_controller.h>
#include <tactility/log.h>

#include <cmath>
#include <initializer_list>

#include <RadioLib.h>

#include <esp_random.h>

#define TAG "cc1101"

namespace {

// TX-done wait is derived from the packet's time-on-air. The margin covers calibration,
// PA ramp and polling latency.
constexpr auto CC1101_TX_TIMEOUT_MARGIN_MILLIS = 500;
constexpr auto CC1101_TX_POLL_INTERVAL_MILLIS = 1;
constexpr uint32_t CC1101_INTERRUPT_BIT = (1 << 0);
constexpr uint32_t CC1101_GDO0_EVENT_BIT = (1 << 1);
constexpr uint32_t CC1101_QUEUED_TX_BIT = (1 << 2);
constexpr uint32_t CC1101_REQUEST_BIT = (1 << 3);
constexpr uint32_t CC1101_RESPONSE_BIT = (1 << 0);
constexpr auto CC1101_REQUEST_TIMEOUT_MILLIS = 1000;
constexpr auto CC1101_LBT_MAX_ATTEMPTS = 5;
constexpr auto CC1101_LBT_BACKOFF_MIN_MILLIS = 20;
constexpr auto CC1101_LBT_BACKOFF_MAX_MILLIS = 200;
// PKTSTATUS bit 3: sync word received, cleared at the end of the packet
constexpr uint8_t CC1101_PKTSTATUS_SFD = (1 << 3);
constexpr size_t CC1101_THREAD_STACK_SIZE = 8192;
// Sync word (16 bits) plus the length byte and CRC (16 bits)
constexpr uint32_t CC1101_PACKET_OVERHEAD_BITS = 16 + 8 + 16;

const char* toString(enum RadioState state) {
    switch (state) {
        case RADIO_STATE_OFF:
            return "off";
        case RADIO_STATE_ON_PENDING:
            return "on-pending";
        case RADIO_STATE_ON:
            return "on";
        case RADIO_STATE_OFF_PENDING:
            return "off-pending";
        case RADIO_STATE_ERROR:
            return "error";
        default:
            return "unknown";
    }
}

const char* toString(enum RadioModulation modulation) {
    switch (modulation) {
        case RADIO_MODULATION_NONE:
            return "none";
        case RADIO_MODULATION_FSK:
            return "FSK";
        case RADIO_MODULATION_FSK4:
            return "4-FSK";
        case RADIO_MODULATION_OOK:
            return "OOK";
        default:
            return "unknown";
    }
}

template<typename T>
constexpr error_t checkLimitsAndApply(T& target, const int32_t value, const int32_t lower, const int32_t upper) {
    if ((value >= lower) && (value <= upper)) {
        target = static_cast<T>(value);
        return ERROR_NONE;
    }
    return ERROR_OUT_OF_RANGE;
}

template<typename T>
constexpr error_t checkValuesAndApply(T& target, const int32_t value, std::initializer_list<int32_t> valids) {
    for (int32_t valid : valids) {
        if (value == valid) {
            target = static_cast<T>(value);
            return ERROR_NONE;
        }
    }
    return ERROR_OUT_OF_RANGE;
}

bool isInLowBand(int32_t frequency) { return (frequency >= 300000000) && (frequency <= 348000000); }
bool isInMidBand(int32_t frequency) { return (frequency >= 387000000) && (frequency <= 464000000); }
bool isInHighBand(int32_t frequency) { return (frequency >= 779000000) && (frequency <= 928000000); }

/** Exposes the protected register access that RadioLib keeps behind RADIOLIB_LOW_LEVEL */
class Cc1101Chip final : public CC1101 {
public:
    explicit Cc1101Chip(::Module* module) : CC1101(module) {}

    uint8_t getMarcState() {
        return static_cast<uint8_t>(SPIgetRegValue(RADIOLIB_CC1101_REG_MARCSTATE, 4, 0));
    }

    /** Instantaneous RSSI in dBm, valid while in RX */
    float getInstantRssi() {
        const auto raw = static_cast<uint8_t>(SPIgetRegValue(RADIOLIB_CC1101_REG_RSSI));
        const float half = (raw >= 128) ? (static_cast<float>(raw) - 256.0f) / 2.0f : static_cast<float>(raw) / 2.0f;
        return half - 74.0f;
    }

    uint8_t getPacketStatus() {
        return static_cast<uint8_t>(SPIgetRegValue(RADIOLIB_CC1101_REG_PKTSTATUS));
    }

    /** Read the VERSION register without running begin() */
    int16_t readVersion() {
        auto* module = getMod();
        module->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_CC1101_CMD_READ;
        module->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_CC1101_CMD_WRITE;
        module->init();
        const int16_t version = getChipVersion();
        module->term();
        return version;
    }
};

} // namespace

static bool isKnownVersion(int16_t version) {
    return (version == RADIOLIB_CC1101_VERSION_CURRENT) ||
        (version == RADIOLIB_CC1101_VERSION_LEGACY) ||
        (version == RADIOLIB_CC1101_VERSION_CLONE);
}

struct Cc1101Radio::RadioParts {
    Cc1101RadiolibHal hal;
    ::Module radioModule;
    Cc1101Chip radio;

    explicit RadioParts(const Settings& settings)
        : hal(settings.spi_host, settings.spi_frequency_hz, settings.spi_controller)
        , radioModule(&hal, settings.pin_cs, settings.pin_gdo0, RADIOLIB_NC, RADIOLIB_NC)
        , radio(&radioModule) {}
};

Cc1101Radio::Cc1101Radio(const Settings& settings)
    : settings(settings) {
    recursive_mutex_construct(&mutex);
    mutex_construct(&requestMutex);
    event_group_construct(&events);
    event_group_construct(&responses);
    parts = new RadioParts(settings);
}

Cc1101Radio::~Cc1101Radio() {
    setEnabled(false);
    delete parts;
    event_group_destruct(&responses);
    event_group_destruct(&events);
    mutex_destruct(&requestMutex);
    recursive_mutex_destruct(&mutex);
}

error_t Cc1101Radio::probe() {
    const int16_t version = parts->radio.readVersion();
    if (!isKnownVersion(version)) {
        LOG_E(TAG, "Probe failed: unexpected VERSION register value 0x%02X (module absent or unpowered?)", (unsigned)version);
        return ERROR_RESOURCE;
    }

    LOG_I(TAG, "Probe OK: CC1101 version 0x%02X", (unsigned)version);
    return ERROR_NONE;
}

// region Thread lifecycle

void Cc1101Radio::gdo0Isr(void* context) {
    auto* self = static_cast<Cc1101Radio*>(context);
    // GDO0 is armed as a HIGH_LEVEL interrupt (edge types are unreliable on the
    // ESP32 per erratum 3.11). A level interrupt re-fires for as long as the line
    // is asserted, so mask it here and let the radio thread re-arm it after the
    // next startReceive().
    gpio_descriptor_disable_interrupt(self->settings.gdo0);
    event_group_set(self->events, CC1101_GDO0_EVENT_BIT);
}

int32_t Cc1101Radio::threadMainStatic(void* context) {
    return static_cast<Cc1101Radio*>(context)->threadMain();
}

bool Cc1101Radio::isThreadInterrupted() const {
    lock();
    const bool interrupted = threadInterrupted;
    unlock();
    return interrupted;
}

int32_t Cc1101Radio::threadMain() {
    int rc = doBegin(getModulation());
    if (rc != 0) {
        return rc;
    }
    setState(RADIO_STATE_ON);

    while (!isThreadInterrupted()) {
        const bool hasRx = doListen();

        if (isThreadInterrupted()) {
            break;
        }

        // Service a received packet before transmitting: an RX-done and a queued TX
        // can coincide in the same iteration.
        if (hasRx) {
            doReceive();
        }
        if (getTxQueueSize() > 0) {
            doTransmit();
        }
    }

    doEnd();
    return 0;
}

error_t Cc1101Radio::setEnabled(bool enabled) {
    lock();

    if (enabled) {
        if ((thread != nullptr) && (thread_get_state(thread) != THREAD_STATE_STOPPED)) {
            LOG_W(TAG, "Already started");
            unlock();
            return ERROR_NONE;
        }

        if (modulation == RADIO_MODULATION_NONE) {
            LOG_E(TAG, "Cannot enable without a modulation set");
            unlock();
            return ERROR_INVALID_STATE;
        }

        if (thread != nullptr) {
            thread_free(thread);
            thread = nullptr;
        }

        threadInterrupted = false;
        setState(RADIO_STATE_ON_PENDING);

        thread = thread_alloc_full("CC1101", CC1101_THREAD_STACK_SIZE, threadMainStatic, this, tskNO_AFFINITY);
        if (thread == nullptr) {
            setState(RADIO_STATE_ERROR);
            unlock();
            return ERROR_OUT_OF_MEMORY;
        }
        thread_set_priority(thread, THREAD_PRIORITY_HIGH);

        if (thread_start(thread) != ERROR_NONE) {
            thread_free(thread);
            thread = nullptr;
            setState(RADIO_STATE_ERROR);
            unlock();
            return ERROR_UNDEFINED;
        }

        unlock();
        return ERROR_NONE;
    } else {
        setState(RADIO_STATE_OFF_PENDING);

        if (thread != nullptr) {
            threadInterrupted = true;
            event_group_set(events, CC1101_INTERRUPT_BIT);

            Thread* oldThread = thread;
            thread = nullptr;

            if (thread_get_state(oldThread) != THREAD_STATE_STOPPED) {
                // Unlock so the thread can lock
                unlock();
                thread_join(oldThread, portMAX_DELAY, pdMS_TO_TICKS(10));
                lock();
            }

            thread_free(oldThread);
        }

        setState(RADIO_STATE_OFF);
        unlock();
        return ERROR_NONE;
    }
}

// endregion

// region State and modulation

enum RadioState Cc1101Radio::getState() const {
    lock();
    const auto result = state;
    unlock();
    return result;
}

void Cc1101Radio::setState(enum RadioState newState) {
    lock();
    if (state == newState) {
        unlock();
        return;
    }
    LOG_I(TAG, "State: %s -> %s", toString(state), toString(newState));
    state = newState;
    unlock();

    radio_state_event_emit(settings.device, newState);
}

error_t Cc1101Radio::setModulation(enum RadioModulation newModulation) {
    const auto currentState = getState();
    if ((currentState == RADIO_STATE_ON_PENDING) || (currentState == RADIO_STATE_ON)) {
        return ERROR_INVALID_STATE;
    }

    if (!((newModulation == RADIO_MODULATION_NONE) || canTransmit(newModulation) || canReceive(newModulation))) {
        return ERROR_NOT_SUPPORTED;
    }

    lock();
    LOG_I(TAG, "Modulation set to %s", toString(newModulation));
    modulation = newModulation;
    unlock();
    return ERROR_NONE;
}

enum RadioModulation Cc1101Radio::getModulation() const {
    lock();
    const auto result = modulation;
    unlock();
    return result;
}

// endregion

// region TX queue

size_t Cc1101Radio::getTxQueueSize() const {
    lock();
    const auto size = txQueue.size();
    unlock();
    return size;
}

Cc1101Radio::TxItem Cc1101Radio::popNextQueuedTx() {
    lock();
    auto tx = std::move(txQueue.front());
    txQueue.pop_front();
    unlock();
    return tx;
}

error_t Cc1101Radio::transmit(const uint8_t* data, size_t length, RadioTxId* id) {
    if ((length == 0) || (length > CC1101_MAX_PACKET_LENGTH)) {
        return ERROR_OUT_OF_RANGE;
    }

    lock();
    const auto txId = lastTxId;
    lastTxId++;
    txQueue.push_back(TxItem {.id = txId, .data = std::vector<uint8_t>(data, data + length)});
    LOG_D(TAG, "TX id=%d queued: %u bytes (queue depth %u)", (int)txId, (unsigned)length, (unsigned)txQueue.size());
    unlock();

    radio_tx_event_emit(settings.device, txId, RADIO_TRANSMISSION_STATE_QUEUED);
    event_group_set(events, CC1101_QUEUED_TX_BIT);

    if (id != nullptr) {
        *id = txId;
    }
    return ERROR_NONE;
}

// endregion

// region Parameters

error_t Cc1101Radio::setParameter(enum RadioParameter parameter, int32_t value) {
    lock();

    error_t result;
    switch (parameter) {
        case RADIO_PARAMETER_POWER:
            // The PA table only holds these output levels
            result = checkValuesAndApply(power, value, {-30, -20, -15, -10, 0, 5, 7, 10});
            break;
        case RADIO_PARAMETER_FREQUENCY:
            if (isInLowBand(value) || isInMidBand(value) || isInHighBand(value)) {
                frequency = value;
                result = ERROR_NONE;
            } else {
                result = ERROR_OUT_OF_RANGE;
            }
            break;
        case RADIO_PARAMETER_BANDWIDTH:
            // RX channel filter bandwidths, rounded to whole kHz
            result = checkValuesAndApply(bandwidth, value, {58000, 68000, 81000, 102000, 116000, 135000, 162000, 203000, 232000, 270000, 325000, 406000, 464000, 541000, 650000, 812000});
            break;
        case RADIO_PARAMETER_PREAMBLE_LENGTH:
            result = checkValuesAndApply(preambleLength, value, {16, 24, 32, 48, 64, 96, 128, 192});
            break;
        case RADIO_PARAMETER_SYNC_WORD:
            // 16 bits, and the chip rejects a zero byte in either half
            if ((value >= 0x0101) && (value <= 0xFFFF) && ((value & 0xFF) != 0) && ((value >> 8) != 0)) {
                syncWord = static_cast<uint16_t>(value);
                result = ERROR_NONE;
            } else {
                result = ERROR_OUT_OF_RANGE;
            }
            break;
        case RADIO_PARAMETER_DATA_RATE:
            switch (modulation) {
                case RADIO_MODULATION_FSK4:
                    result = checkLimitsAndApply(bitRate, value, 600, 300000);
                    break;
                case RADIO_MODULATION_OOK:
                    result = checkLimitsAndApply(bitRate, value, 600, 250000);
                    break;
                default:
                    result = checkLimitsAndApply(bitRate, value, 600, 500000);
                    break;
            }
            break;
        case RADIO_PARAMETER_FREQUENCY_DEVIATION:
            if (modulation == RADIO_MODULATION_OOK) {
                result = ERROR_NOT_SUPPORTED;
            } else {
                result = checkLimitsAndApply(frequencyDeviation, value, 1587, 380800);
            }
            break;
        case RADIO_PARAMETER_LBT:
            result = checkLimitsAndApply(lbt, value, 0, 1);
            break;
        case RADIO_PARAMETER_LBT_RSSI_THRESHOLD:
            result = checkLimitsAndApply(lbtRssiThreshold, value, -140, 0);
            break;
        default:
            result = ERROR_NOT_SUPPORTED;
            break;
    }

    if (result == ERROR_NONE) {
        LOG_D(TAG, "Parameter %d = %d", (int)parameter, (int)value);
    }

    unlock();
    return result;
}

error_t Cc1101Radio::getParameter(enum RadioParameter parameter, int32_t* value) const {
    lock();

    error_t result = ERROR_NONE;
    switch (parameter) {
        case RADIO_PARAMETER_POWER:
            *value = power;
            break;
        case RADIO_PARAMETER_FREQUENCY:
            *value = frequency;
            break;
        case RADIO_PARAMETER_BANDWIDTH:
            *value = bandwidth;
            break;
        case RADIO_PARAMETER_PREAMBLE_LENGTH:
            *value = preambleLength;
            break;
        case RADIO_PARAMETER_SYNC_WORD:
            *value = syncWord;
            break;
        case RADIO_PARAMETER_DATA_RATE:
            *value = bitRate;
            break;
        case RADIO_PARAMETER_FREQUENCY_DEVIATION:
            if (modulation == RADIO_MODULATION_OOK) {
                result = ERROR_NOT_SUPPORTED;
            } else {
                *value = frequencyDeviation;
            }
            break;
        case RADIO_PARAMETER_LBT:
            *value = lbt;
            break;
        case RADIO_PARAMETER_LBT_RSSI_THRESHOLD:
            *value = lbtRssiThreshold;
            break;
        default:
            result = ERROR_NOT_SUPPORTED;
            break;
    }

    unlock();
    return result;
}

// endregion

// region Radio operations (radio thread only)

// GDO0 uses the GPIO descriptor callback API in HIGH_LEVEL mode. RadioLib leaves GDO0
// in high impedance outside of reception, so the interrupt is only enabled after
// startReceive() has mapped GDO0 to "RX FIFO full or end of packet", and the pull-down
// keeps the floating line low in between.
void Cc1101Radio::registerGdo0Isr() {
    gpio_flags_t flags = GPIO_FLAG_DIRECTION_INPUT | GPIO_FLAG_PULL_DOWN;
    flags = GPIO_FLAG_INTERRUPT_TO_OPTIONS(flags, GPIO_INTERRUPT_HIGH_LEVEL);
    if (gpio_descriptor_set_flags(settings.gdo0, flags) != ERROR_NONE ||
        gpio_descriptor_add_callback(settings.gdo0, gdo0Isr, this) != ERROR_NONE) {
        LOG_E(TAG, "Failed to install GDO0 interrupt");
    }
    gpio_descriptor_disable_interrupt(settings.gdo0);
}

void Cc1101Radio::unregisterGdo0Isr() {
    gpio_descriptor_disable_interrupt(settings.gdo0);
    gpio_descriptor_remove_callback(settings.gdo0);
}

void Cc1101Radio::applyBandSwitch() {
    uint8_t value;
    if (isInLowBand(frequency)) {
        value = settings.band_switch_low;
    } else if (isInMidBand(frequency)) {
        value = settings.band_switch_mid;
    } else {
        value = settings.band_switch_high;
    }

    if (settings.band_switch_0 != nullptr) {
        gpio_descriptor_set_level(settings.band_switch_0, (value & 0b01) != 0);
    }
    if (settings.band_switch_1 != nullptr) {
        gpio_descriptor_set_level(settings.band_switch_1, (value & 0b10) != 0);
    }
}

uint32_t Cc1101Radio::getTimeOnAirMillis(size_t length) const {
    const uint32_t bits = preambleLength + CC1101_PACKET_OVERHEAD_BITS + (length * 8);
    // 4-FSK carries 2 bits per symbol, but the data rate parameter is in bit/s
    return (bits * 1000) / bitRate + 1;
}

int Cc1101Radio::doBegin(enum RadioModulation beginModulation) {
    auto& radio = parts->radio;

    applyBandSwitch();

    // RadioLib takes MHz/kHz/kbps floats; the driver stores Hz/bit/s integers.
    ConfigFSK_t config;
    config.frequency = static_cast<float>(frequency) / 1000000.0f;
    config.bitRate = static_cast<float>(bitRate) / 1000.0f;
    config.frequencyDeviation = static_cast<float>(frequencyDeviation) / 1000.0f;
    config.receiverBandwidth = static_cast<float>(bandwidth) / 1000.0f;
    config.power = power;
    config.preambleLength = preambleLength;

    LOG_I(
        TAG,
        "Starting %s: %.3f MHz, %.2f kbps, deviation %.1f kHz, BW %.0f kHz, preamble %u, sync 0x%04X, %d dBm",
        toString(beginModulation),
        config.frequency,
        config.bitRate,
        config.frequencyDeviation,
        config.receiverBandwidth,
        preambleLength,
        syncWord,
        power
    );

    int16_t rc;
    if (beginModulation == RADIO_MODULATION_FSK4) {
        rc = radio.beginFSK4(config);
    } else if ((beginModulation == RADIO_MODULATION_FSK) || (beginModulation == RADIO_MODULATION_OOK)) {
        rc = radio.begin(config);
        if ((rc == RADIOLIB_ERR_NONE) && (beginModulation == RADIO_MODULATION_OOK)) {
            rc = radio.setOOK(true);
        }
    } else {
        LOG_E(TAG, "CC1101 not capable of modulation \"%s\"", toString(beginModulation));
        setState(RADIO_STATE_ERROR);
        return -1;
    }

    if (rc != RADIOLIB_ERR_NONE) {
        LOG_E(TAG, "RadioLib initialization failed with code %hi", rc);
        setState(RADIO_STATE_ERROR);
        return -1;
    }

    rc = radio.setSyncWord(static_cast<uint8_t>(syncWord >> 8), static_cast<uint8_t>(syncWord & 0xFF));
    if (rc != RADIOLIB_ERR_NONE) {
        LOG_E(TAG, "Setting sync word failed with code %hi", rc);
        setState(RADIO_STATE_ERROR);
        return -1;
    }

    // Longer packets are discarded by the chip, so a received packet always fits the FIFO
    rc = radio.variablePacketLengthMode(CC1101_MAX_PACKET_LENGTH);
    if (rc != RADIOLIB_ERR_NONE) {
        LOG_E(TAG, "Setting packet length mode failed with code %hi", rc);
        setState(RADIO_STATE_ERROR);
        return -1;
    }

    LOG_I(TAG, "Modem initialized");
    registerGdo0Isr();
    return 0;
}

void Cc1101Radio::doEnd() {
    unregisterGdo0Isr();
    // Leave the modem in its lowest-power state. The next enable runs a full begin().
    const int16_t rc = parts->radio.sleep();
    if (rc != RADIOLIB_ERR_NONE) {
        LOG_W(TAG, "Putting modem to sleep failed with code %hi", rc);
    } else {
        LOG_I(TAG, "Modem put to sleep");
    }
}

void Cc1101Radio::doTransmit() {
    auto tx = popNextQueuedTx();
    auto& radio = parts->radio;

    // The chip is still in RX from doListen(), so the channel can be sampled before transmitting
    if (!waitForClearChannel()) {
        if (isThreadInterrupted()) {
            radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_ERROR);
        } else {
            LOG_W(TAG, "TX id=%d: channel busy", (int)tx.id);
            radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_CHANNEL_BUSY);
        }
        return;
    }

    LOG_I(TAG, "TX id=%d: %u bytes (%u more queued)", (int)tx.id, (unsigned)tx.data.size(), (unsigned)getTxQueueSize());
    int16_t rc = radio.startTransmit(tx.data.data(), tx.data.size());
    if (rc != RADIOLIB_ERR_NONE) {
        LOG_E(TAG, "Error transmitting id=%d, rc=%hi", (int)tx.id, rc);
        radio.standby();
        radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_ERROR);
        return;
    }

    radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_TRANSMIT_PENDING);

    // The chip returns to IDLE once the packet is out (TXOFF_MODE = IDLE).
    // Polling MARCSTATE avoids depending on a wired GDO2 line.
    const uint32_t txTimeoutMillis = getTimeOnAirMillis(tx.data.size()) + CC1101_TX_TIMEOUT_MARGIN_MILLIS;
    uint32_t elapsedMillis = 0;
    bool done = false;
    while (elapsedMillis < txTimeoutMillis) {
        if (radio.getMarcState() == RADIOLIB_CC1101_MARC_STATE_IDLE) {
            done = true;
            break;
        }
        if (event_group_wait(events, CC1101_INTERRUPT_BIT, false, false, nullptr, pdMS_TO_TICKS(CC1101_TX_POLL_INTERVAL_MILLIS)) == ERROR_NONE) {
            break;
        }
        elapsedMillis += CC1101_TX_POLL_INTERVAL_MILLIS;
    }

    if (done) {
        radio.finishTransmit();
    } else {
        radio.standby();
    }

    if (isThreadInterrupted()) {
        radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_ERROR);
    } else if (done) {
        LOG_I(TAG, "TX id=%d: done", (int)tx.id);
        radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_TRANSMITTED);
    } else {
        LOG_W(TAG, "TX id=%d: not finished within %u ms", (int)tx.id, (unsigned)txTimeoutMillis);
        radio_tx_event_emit(settings.device, tx.id, RADIO_TRANSMISSION_STATE_TIMEOUT);
    }
}

bool Cc1101Radio::doListen() {
    auto& radio = parts->radio;

    const int16_t rc = radio.startReceive();
    if (rc != RADIOLIB_ERR_NONE) {
        LOG_E(TAG, "Error starting RX, RadioLib returned %hi", rc);
        // Still wait, so a queued TX or a stop request is serviced instead of spinning
        event_group_wait(events, CC1101_INTERRUPT_BIT | CC1101_QUEUED_TX_BIT, false, true, nullptr, portMAX_DELAY);
        return false;
    }

    // GDO0 now signals end of packet, so the interrupt can be armed
    gpio_descriptor_enable_interrupt(settings.gdo0);

    while (true) {
        uint32_t flags = 0;
        event_group_wait(
            events,
            CC1101_INTERRUPT_BIT | CC1101_GDO0_EVENT_BIT | CC1101_QUEUED_TX_BIT | CC1101_REQUEST_BIT,
            false,
            false,
            &flags,
            portMAX_DELAY
        );
        // Requests are served without leaving RX, so a packet in flight isn't lost
        if ((flags & (CC1101_INTERRUPT_BIT | CC1101_GDO0_EVENT_BIT | CC1101_QUEUED_TX_BIT)) == 0) {
            event_group_clear(events, CC1101_REQUEST_BIT);
            handleRequest();
            continue;
        }
        event_group_clear(events, flags & (CC1101_INTERRUPT_BIT | CC1101_GDO0_EVENT_BIT | CC1101_QUEUED_TX_BIT));
        gpio_descriptor_disable_interrupt(settings.gdo0);
        // A packet that arrived after the wake is received before a transmission can discard it
        if ((event_group_get(events) & CC1101_GDO0_EVENT_BIT) != 0) {
            event_group_clear(events, CC1101_GDO0_EVENT_BIT);
            flags |= CC1101_GDO0_EVENT_BIT;
        }
        return (flags & CC1101_GDO0_EVENT_BIT) != 0;
    }
}

void Cc1101Radio::doReceive() {
    auto& radio = parts->radio;

    const size_t rxSize = radio.getPacketLength(true);
    if ((rxSize == 0) || (rxSize > CC1101_MAX_PACKET_LENGTH)) {
        // Spurious IRQ or corrupt length byte
        radio.finishReceive();
        return;
    }

    std::vector<uint8_t> data(rxSize);
    const int16_t rc = radio.readData(data.data(), rxSize);
    if (rc == RADIOLIB_ERR_CRC_MISMATCH) {
        LOG_D(TAG, "RX: dropped packet with CRC mismatch");
    } else if (rc != RADIOLIB_ERR_NONE) {
        LOG_E(TAG, "Error receiving data, RadioLib returned %hi", rc);
    } else {
        const float rssi = radio.getRSSI();
        LOG_I(TAG, "RX: %u bytes, RSSI %.1f dBm, LQI %u", (unsigned)rxSize, rssi, (unsigned)radio.getLQI());
        radio_rx_event_emit(settings.device, data.data(), rxSize, rssi, NAN);
    }
}

bool Cc1101Radio::isChannelBusy() {
    auto& radio = parts->radio;
    return ((radio.getPacketStatus() & CC1101_PKTSTATUS_SFD) != 0) ||
           (radio.getInstantRssi() >= static_cast<float>(lbtRssiThreshold));
}

bool Cc1101Radio::waitForClearChannel() {
    lock();
    const bool enabled = lbt;
    unlock();
    if (!enabled) {
        return true;
    }

    // RSSI is only valid in RX, which a preceding reception may have ended
    auto& radio = parts->radio;
    if (radio.getMarcState() != RADIOLIB_CC1101_MARC_STATE_RX) {
        radio.startReceive();
        delay_millis(1);
    }

    for (int attempt = 0; attempt < CC1101_LBT_MAX_ATTEMPTS; attempt++) {
        if (!isChannelBusy()) {
            return true;
        }
        const uint32_t backoffMillis = CC1101_LBT_BACKOFF_MIN_MILLIS + (esp_random() % (CC1101_LBT_BACKOFF_MAX_MILLIS - CC1101_LBT_BACKOFF_MIN_MILLIS));
        LOG_D(TAG, "Channel busy, backing off %u ms", (unsigned)backoffMillis);
        // Returns early when the radio is being turned off
        if (event_group_wait(events, CC1101_INTERRUPT_BIT, false, false, nullptr, pdMS_TO_TICKS(backoffMillis)) == ERROR_NONE) {
            return false;
        }
    }
    return false;
}

void Cc1101Radio::handleRequest() {
    auto& radio = parts->radio;
    requestResult = ERROR_NONE;
    if (pendingRequest == Request::Present) {
        const int16_t version = radio.getChipVersion();
        requestResult = isKnownVersion(version) ? ERROR_NONE : ERROR_NOT_FOUND;
    } else if (pendingRequest == Request::Rssi) {
        requestRssi = radio.getInstantRssi();
    } else if (pendingRequest == Request::Receiving) {
        requestReceiving = (radio.getPacketStatus() & CC1101_PKTSTATUS_SFD) != 0;
    }
    event_group_set(responses, CC1101_RESPONSE_BIT);
}

error_t Cc1101Radio::performRequest(Request request) {
    if (getState() != RADIO_STATE_ON) {
        return ERROR_INVALID_STATE;
    }

    mutex_lock(&requestMutex);
    pendingRequest = request;
    event_group_clear(responses, CC1101_RESPONSE_BIT);
    event_group_set(events, CC1101_REQUEST_BIT);
    const error_t waitResult = event_group_wait(responses, CC1101_RESPONSE_BIT, false, true, nullptr, pdMS_TO_TICKS(CC1101_REQUEST_TIMEOUT_MILLIS));
    const error_t result = (waitResult == ERROR_NONE) ? requestResult : ERROR_TIMEOUT;
    mutex_unlock(&requestMutex);
    return result;
}

error_t Cc1101Radio::getRssi(float* rssi) {
    const error_t result = performRequest(Request::Rssi);
    if (result == ERROR_NONE) {
        *rssi = requestRssi;
    }
    return result;
}

error_t Cc1101Radio::checkPresent() {
    if (getState() != RADIO_STATE_ON) {
        return ERROR_NONE;
    }
    // A busy or changing radio counts as present
    return (performRequest(Request::Present) == ERROR_NOT_FOUND) ? ERROR_NOT_FOUND : ERROR_NONE;
}

error_t Cc1101Radio::isReceiving(bool* receiving) {
    const error_t result = performRequest(Request::Receiving);
    if (result == ERROR_NONE) {
        *receiving = requestReceiving;
    }
    return result;
}

// endregion
