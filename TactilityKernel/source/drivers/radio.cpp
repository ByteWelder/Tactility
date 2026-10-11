// SPDX-License-Identifier: Apache-2.0
#include <tactility/drivers/radio.h>
#include <tactility/concurrent/mutex.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/time.h>

#include <algorithm>
#include <cstring>

#define RADIO_API(device) ((const struct RadioApi*)device_get_driver(device)->api)

struct RadioEventMutex {
    Mutex handle {};
    RadioEventMutex() { mutex_construct(&handle); }
    ~RadioEventMutex() { mutex_destruct(&handle); }
};

// Each event kind gets its own intrusive singly-linked subscription list, keyed by device, and
// its own coarse-grained mutex. Notifying a subscriber never invokes caller code (just a struct
// copy and a task_event_group_signal), so there is no reentrancy concern requiring a
// snapshot-then-unlock dance.
static RadioStateEventSubscription* radio_state_event_subscriptions = nullptr;
static RadioEventMutex radio_state_event_subscriptions_mutex;

static RadioRxEventSubscription* radio_rx_event_subscriptions = nullptr;
static RadioEventMutex radio_rx_event_subscriptions_mutex;

static RadioTxEventSubscription* radio_tx_event_subscriptions = nullptr;
static RadioEventMutex radio_tx_event_subscriptions_mutex;

extern "C" {

// region State events

error_t radio_state_event_subscribe(RadioStateEventSubscription* sub, TaskEventGroup* event_group, Device* device) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&radio_state_event_subscriptions_mutex.handle);

    // Avoid cyclic subscription list that would loop forever
    for (RadioStateEventSubscription* existing = radio_state_event_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&radio_state_event_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->bit = bit;
    sub->internal.device = device;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = radio_state_event_subscriptions;
    radio_state_event_subscriptions = sub;
    mutex_unlock(&radio_state_event_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t radio_state_event_unsubscribe(RadioStateEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&radio_state_event_subscriptions_mutex.handle);
    for (RadioStateEventSubscription** link = &radio_state_event_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&radio_state_event_subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t radio_state_event_emit(Device* device, enum RadioState state) {
    RadioStateEvent stamped_event { .timestamp = get_micros_since_boot(), .state = state };

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&radio_state_event_subscriptions_mutex.handle);
    for (RadioStateEventSubscription* sub = radio_state_event_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.device != device) {
            continue;
        }

        if (sub->internal.count >= RADIO_STATE_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % RADIO_STATE_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&radio_state_event_subscriptions_mutex.handle);

    return result;
}

error_t radio_state_event_poll(RadioStateEventSubscription* sub, RadioStateEvent* out_event) {
    mutex_lock(&radio_state_event_subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % RADIO_STATE_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&radio_state_event_subscriptions_mutex.handle);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

// region RX events

error_t radio_rx_event_subscribe(RadioRxEventSubscription* sub, TaskEventGroup* event_group, Device* device) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&radio_rx_event_subscriptions_mutex.handle);

    // Avoid cyclic subscription list that would loop forever
    for (RadioRxEventSubscription* existing = radio_rx_event_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&radio_rx_event_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->bit = bit;
    sub->internal.device = device;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = radio_rx_event_subscriptions;
    radio_rx_event_subscriptions = sub;
    mutex_unlock(&radio_rx_event_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t radio_rx_event_unsubscribe(RadioRxEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&radio_rx_event_subscriptions_mutex.handle);
    for (RadioRxEventSubscription** link = &radio_rx_event_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&radio_rx_event_subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t radio_rx_event_emit(Device* device, const uint8_t* data, size_t length, float rssi, float snr) {
    RadioRxEvent stamped_event {};
    stamped_event.timestamp = get_micros_since_boot();
    stamped_event.length = std::min(length, sizeof(stamped_event.data));
    std::memcpy(stamped_event.data, data, stamped_event.length);
    stamped_event.rssi = rssi;
    stamped_event.snr = snr;

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&radio_rx_event_subscriptions_mutex.handle);
    for (RadioRxEventSubscription* sub = radio_rx_event_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.device != device) {
            continue;
        }

        if (sub->internal.count >= RADIO_RX_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % RADIO_RX_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&radio_rx_event_subscriptions_mutex.handle);

    return result;
}

error_t radio_rx_event_poll(RadioRxEventSubscription* sub, RadioRxEvent* out_event) {
    mutex_lock(&radio_rx_event_subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % RADIO_RX_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&radio_rx_event_subscriptions_mutex.handle);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

// region TX events

error_t radio_tx_event_subscribe(RadioTxEventSubscription* sub, TaskEventGroup* event_group, Device* device) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&radio_tx_event_subscriptions_mutex.handle);

    // Avoid cyclic subscription list that would loop forever
    for (RadioTxEventSubscription* existing = radio_tx_event_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&radio_tx_event_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->bit = bit;
    sub->internal.device = device;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = radio_tx_event_subscriptions;
    radio_tx_event_subscriptions = sub;
    mutex_unlock(&radio_tx_event_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t radio_tx_event_unsubscribe(RadioTxEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&radio_tx_event_subscriptions_mutex.handle);
    for (RadioTxEventSubscription** link = &radio_tx_event_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&radio_tx_event_subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t radio_tx_event_emit(Device* device, RadioTxId id, enum RadioTransmissionState state) {
    RadioTxEvent stamped_event { .timestamp = get_micros_since_boot(), .id = id, .state = state };

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&radio_tx_event_subscriptions_mutex.handle);
    for (RadioTxEventSubscription* sub = radio_tx_event_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.device != device) {
            continue;
        }

        if (sub->internal.count >= RADIO_TX_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % RADIO_TX_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&radio_tx_event_subscriptions_mutex.handle);

    return result;
}

error_t radio_tx_event_poll(RadioTxEventSubscription* sub, RadioTxEvent* out_event) {
    mutex_lock(&radio_tx_event_subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % RADIO_TX_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&radio_tx_event_subscriptions_mutex.handle);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

error_t radio_get_state(struct Device* device, enum RadioState* state) {
    return RADIO_API(device)->get_radio_state(device, state);
}

error_t radio_set_enabled(struct Device* device, bool enabled) {
    return RADIO_API(device)->set_enabled(device, enabled);
}

error_t radio_set_modulation(struct Device* device, enum RadioModulation modulation) {
    return RADIO_API(device)->set_modulation(device, modulation);
}

error_t radio_get_modulation(struct Device* device, enum RadioModulation* modulation) {
    return RADIO_API(device)->get_modulation(device, modulation);
}

bool radio_can_transmit(struct Device* device, enum RadioModulation modulation) {
    return RADIO_API(device)->can_transmit(device, modulation);
}

bool radio_can_receive(struct Device* device, enum RadioModulation modulation) {
    return RADIO_API(device)->can_receive(device, modulation);
}

error_t radio_set_parameter(struct Device* device, enum RadioParameter parameter, int32_t value) {
    return RADIO_API(device)->set_parameter(device, parameter, value);
}

error_t radio_get_parameter(struct Device* device, enum RadioParameter parameter, int32_t* value) {
    return RADIO_API(device)->get_parameter(device, parameter, value);
}

error_t radio_transmit(struct Device* device, const uint8_t* data, size_t length, RadioTxId* id) {
    return RADIO_API(device)->transmit(device, data, length, id);
}

const struct DeviceType RADIO_TYPE = {
    .name = "radio"
};

} // extern "C"
