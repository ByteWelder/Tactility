// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tactility/concurrent/task_event_group.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Device;

/**
 * Device type and API for sub-GHz packet transceivers such as the Semtech SX126x family.
 * The API is LoRa-centric but exposes the modem's other modulation schemes as well.
 */

enum LoraModulation {
    LORA_MODULATION_NONE = 0,
    LORA_MODULATION_FSK,
    LORA_MODULATION_LORA,
    LORA_MODULATION_LR_FHSS,
};

/**
 * Tunable radio parameters. Values are int32_t; the unit is documented per parameter.
 * Frequencies and rates use their base SI unit (Hz, bit/s) on purpose: a float can't hold
 * a value like 906875000 Hz (906.875 MHz) exactly, so integers avoid the rounding a
 * fractional MHz/kHz representation would introduce.
 *
 * Which parameters are available depends on the driver and the selected modulation.
 */
enum LoraParameter {
    /** TX output power in dBm */
    LORA_PARAMETER_POWER = 0,
    /** Boosted RX gain mode: 0 = off, 1 = on */
    LORA_PARAMETER_BOOSTED_GAIN,
    /** Carrier frequency in Hz */
    LORA_PARAMETER_FREQUENCY,
    /** Bandwidth in Hz */
    LORA_PARAMETER_BANDWIDTH,
    /** LoRa spreading factor (7-12) */
    LORA_PARAMETER_SPREADING_FACTOR,
    /** LoRa coding rate denominator (5-8 for 4/5 to 4/8) */
    LORA_PARAMETER_CODING_RATE,
    /**
     * LoRa sync word (0x00-0xFF). Distinguishes otherwise-identical networks: a receiver
     * only accepts packets whose sync word matches. Conventionally 0x12 for private
     * networks and 0x34 for public/LoRaWAN. LoRa modulation only.
     */
    LORA_PARAMETER_SYNC_WORD,
    /** Preamble length in symbols (LoRa) or bits (FSK) */
    LORA_PARAMETER_PREAMBLE_LENGTH,
    /** FSK frequency deviation from the carrier in Hz */
    LORA_PARAMETER_FREQUENCY_DEVIATION,
    /** FSK bit rate in bits per second */
    LORA_PARAMETER_DATA_RATE,
    /** LR-FHSS grid spacing: 0 = 25 kHz (wide), 1 = 3.9 kHz (narrow) */
    LORA_PARAMETER_NARROW_GRID,
    /**
     * PA over-current protection limit in mA. Fail-safe: a low limit caps the current the
     * PA can push into a bad or disconnected antenna, but also caps the achievable output
     * power. Drivers keep a conservative default; a board-aware consumer that knows its
     * antenna and PA can raise it to reach full output power.
     */
    LORA_PARAMETER_CURRENT_LIMIT,
};

enum LoraRadioState {
    LORA_RADIO_STATE_OFF,
    LORA_RADIO_STATE_ON_PENDING,
    LORA_RADIO_STATE_ON,
    LORA_RADIO_STATE_OFF_PENDING,
    LORA_RADIO_STATE_ERROR,
};

/** Identifies a queued transmission. Unique per device until it wraps around. */
typedef int32_t LoraTxId;

enum LoraTransmissionState {
    /** Accepted into the TX queue */
    LORA_TRANSMISSION_STATE_QUEUED,
    /** Handed to the modem, waiting for TX-done */
    LORA_TRANSMISSION_STATE_TRANSMIT_PENDING,
    /** TX-done confirmed by the modem */
    LORA_TRANSMISSION_STATE_TRANSMITTED,
    /** No TX-done within the driver's timeout */
    LORA_TRANSMISSION_STATE_TIMEOUT,
    /** The modem rejected the transmission */
    LORA_TRANSMISSION_STATE_ERROR,
};

/** Largest RX payload any supported modem can hand back in one packet. */
#define LORA_RX_MAX_PACKET_LENGTH 255

struct LoraApi {
    /**
     * Get the radio state of the device.
     * @param[in] device the lora device
     * @param[out] state the radio state
     * @return ERROR_NONE on success
     */
    error_t (*get_radio_state)(struct Device* device, enum LoraRadioState* state);

    /**
     * Turn the radio on or off. Requires a modulation to be set before enabling.
     * Turning on is asynchronous: observe the radio state to know when it's up.
     * @param[in] device the lora device
     * @param[in] enabled true to turn the radio on
     * @return ERROR_NONE on success
     * @retval ERROR_INVALID_STATE when enabling without a modulation set
     */
    error_t (*set_enabled)(struct Device* device, bool enabled);

    /**
     * Set the modulation scheme. Only allowed while the radio is off.
     * @param[in] device the lora device
     * @param[in] modulation the modulation scheme
     * @return ERROR_NONE on success
     * @retval ERROR_INVALID_STATE when the radio is on or turning on
     * @retval ERROR_NOT_SUPPORTED when the device supports neither TX nor RX for this modulation
     */
    error_t (*set_modulation)(struct Device* device, enum LoraModulation modulation);

    /**
     * Get the current modulation scheme.
     * @param[in] device the lora device
     * @param[out] modulation the modulation scheme
     * @return ERROR_NONE on success
     */
    error_t (*get_modulation)(struct Device* device, enum LoraModulation* modulation);

    /**
     * @param[in] device the lora device
     * @param[in] modulation the modulation scheme
     * @return true when the device can transmit with the given modulation
     */
    bool (*can_transmit)(struct Device* device, enum LoraModulation modulation);

    /**
     * @param[in] device the lora device
     * @param[in] modulation the modulation scheme
     * @return true when the device can receive with the given modulation
     */
    bool (*can_receive)(struct Device* device, enum LoraModulation modulation);

    /**
     * Set a radio parameter. See LoraParameter for units.
     * Parameters apply to the current modulation and take effect the next time the radio turns on.
     * @param[in] device the lora device
     * @param[in] parameter the parameter to set
     * @param[in] value the value to set
     * @return ERROR_NONE on success
     * @retval ERROR_NOT_SUPPORTED when the parameter doesn't apply to the device or modulation
     * @retval ERROR_OUT_OF_RANGE when the value is invalid for the parameter
     */
    error_t (*set_parameter)(struct Device* device, enum LoraParameter parameter, int32_t value);

    /**
     * Get a radio parameter. See LoraParameter for units.
     * @param[in] device the lora device
     * @param[in] parameter the parameter to get
     * @param[out] value the current value
     * @return ERROR_NONE on success
     * @retval ERROR_NOT_SUPPORTED when the parameter doesn't apply to the device or modulation
     */
    error_t (*get_parameter)(struct Device* device, enum LoraParameter parameter, int32_t* value);

    /**
     * Queue a packet for transmission. The data is copied.
     * Progress is reported through the TX callbacks, starting with QUEUED.
     * @param[in] device the lora device
     * @param[in] data the packet payload
     * @param[in] length the payload length in bytes
     * @param[out] id the id assigned to this transmission (optional, can be NULL)
     * @return ERROR_NONE on success
     */
    error_t (*transmit)(struct Device* device, const uint8_t* data, size_t length, LoraTxId* id);
};

/** Identifies the kind of lora event delivered through lora_event_poll(). */
enum LoraEventType {
    LORA_EVENT_STATE, // struct LoraStateEventData
    LORA_EVENT_RX,    // struct LoraRxEventData
    LORA_EVENT_TX,    // struct LoraTxEventData
};

/** Data for LORA_EVENT_STATE. */
struct LoraStateEventData {
    enum LoraRadioState state;
};

/** Data for LORA_EVENT_RX. */
struct LoraRxEventData {
    uint8_t data[LORA_RX_MAX_PACKET_LENGTH];
    size_t length;
    /** Received signal strength in dBm */
    float rssi;
    /** Signal-to-noise ratio in dB */
    float snr;
};

/** Data for LORA_EVENT_TX. */
struct LoraTxEventData {
    LoraTxId id;
    enum LoraTransmissionState state;
};

struct LoraEvent {
    enum LoraEventType type;
    /** Stamped by lora_event_emit(); any value passed in by the caller is ignored. */
    uint64_t timestamp;
    union {
        struct LoraStateEventData state;
        struct LoraRxEventData rx;
        struct LoraTxEventData tx;
    } data;
};

/**
 * Number of events that can be queued per subscription before lora_event_emit() starts
 * returning ERROR_RESOURCE (dropping the newest event, preserving FIFO order of what's
 * already queued).
 */
#define LORA_EVENT_QUEUE_CAPACITY 8

/**
 * Caller-owned subscription node, registered with lora_event_subscribe() and drained with
 * lora_event_poll(). Events queue by value (FIFO): a poll subscription that falls behind loses
 * the oldest un-popped RX packet's data only once the queue is full, not on every new event.
 * @warning Fields other than `bit` are for internal use only; do not read or write them directly.
 */
struct LoraEventSubscription {
    /** Set by lora_event_subscribe(). Read-only for the caller: OR it into a
     * task_event_group_wait() mask (alongside other subscriptions sharing the same
     * `event_group`) to block on this subscription and other event sources with one call. */
    uint32_t bit;

    struct {
        /** The lora device this subscription receives events for; set by lora_event_subscribe(). */
        struct Device* device;

        /** Caller-owned, borrowed; set by lora_event_subscribe(). */
        struct TaskEventGroup* event_group;

        struct LoraEvent queue[LORA_EVENT_QUEUE_CAPACITY];
        uint8_t head;
        uint8_t count;

        struct LoraEventSubscription* next;
    } internal;
};

/**
 * Register a subscription for events emitted by @a device.
 * @warning Does not work in ISR context.
 * @param[in,out] sub subscription to register; owns the storage, must stay alive (and
 * stationary) until unsubscribed
 * @param[in] event_group caller-owned group to wait on; must outlive @a sub (i.e. be
 * destructed only after lora_event_unsubscribe()). To block for an event, call
 * task_event_group_wait()/task_event_group_wait_any() on this group (OR sub->bit into the mask,
 * or use _wait_any() to include every subscription sharing it), then drain with lora_event_poll().
 * @param[in] device the lora device this subscription receives events for
 * @retval ERROR_NONE on success
 * @retval ERROR_RESOURCE @a event_group has no free bits left to claim; @a sub was not registered
 * @retval ERROR_INVALID_STATE @a sub is already registered
 */
error_t lora_event_subscribe(struct LoraEventSubscription* sub, struct TaskEventGroup* event_group, struct Device* device);

/**
 * Remove a previously registered subscription.
 * @warning Does not work in ISR context.
 * @return ERROR_NONE on success, ERROR_NOT_FOUND if no matching subscription exists
 */
error_t lora_event_unsubscribe(struct LoraEventSubscription* sub);

/**
 * Non-blocking: pop the next event for @a sub if one is already queued.
 * @warning Never blocks. To wait for an event, block in task_event_group_wait()/
 * task_event_group_wait_any() on @a sub's event group first (see lora_event_subscribe()), then
 * drain with this in a loop.
 * @retval ERROR_NONE @a out_event was filled
 * @retval ERROR_TIMEOUT nothing queued right now
 */
error_t lora_event_poll(struct LoraEventSubscription* sub, struct LoraEvent* out_event);

/**
 * Emit a lora event to every subscriber of @a device. Called by LoraApi driver
 * implementations; not intended for consumers of the API.
 * @param[in] device the lora device the event originates from
 * @param[in] event the event to emit; @a event->timestamp is overwritten
 * @return ERROR_NONE on success, ERROR_NOT_FOUND if @a device has no subscribers
 */
error_t lora_event_emit(struct Device* device, const struct LoraEvent* event);

extern const struct DeviceType LORA_TYPE;

/** @return the first registered lora device, regardless of started state, or NULL if none exists */
struct Device* lora_find_first_registered_device(void);

error_t lora_get_radio_state(struct Device* device, enum LoraRadioState* state);
error_t lora_set_enabled(struct Device* device, bool enabled);
error_t lora_set_modulation(struct Device* device, enum LoraModulation modulation);
error_t lora_get_modulation(struct Device* device, enum LoraModulation* modulation);
bool lora_can_transmit(struct Device* device, enum LoraModulation modulation);
bool lora_can_receive(struct Device* device, enum LoraModulation modulation);
error_t lora_set_parameter(struct Device* device, enum LoraParameter parameter, int32_t value);
error_t lora_get_parameter(struct Device* device, enum LoraParameter parameter, int32_t* value);
error_t lora_transmit(struct Device* device, const uint8_t* data, size_t length, LoraTxId* id);

#ifdef __cplusplus
}
#endif
