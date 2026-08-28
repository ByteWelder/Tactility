// SPDX-License-Identifier: Apache-2.0
#include <tactility/drivers/lora.h>
#include <tactility/concurrent/mutex.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/time.h>

#include <algorithm>
#include <cstring>

#define LORA_API(device) ((const struct LoraApi*)device_get_driver(device)->api)

struct LoraEventMutex {
    Mutex handle {};
    LoraEventMutex() { mutex_construct(&handle); }
    ~LoraEventMutex() { mutex_destruct(&handle); }
};

// Each event kind gets its own intrusive singly-linked subscription list, keyed by device, and
// its own coarse-grained mutex. Notifying a subscriber never invokes caller code (just a struct
// copy and a task_event_group_signal), so there is no reentrancy concern requiring a
// snapshot-then-unlock dance.
static LoraStateEventSubscription* lora_state_event_subscriptions = nullptr;
static LoraEventMutex lora_state_event_subscriptions_mutex;

static LoraRxEventSubscription* lora_rx_event_subscriptions = nullptr;
static LoraEventMutex lora_rx_event_subscriptions_mutex;

static LoraTxEventSubscription* lora_tx_event_subscriptions = nullptr;
static LoraEventMutex lora_tx_event_subscriptions_mutex;

extern "C" {

// region State events

error_t lora_state_event_subscribe(LoraStateEventSubscription* sub, TaskEventGroup* event_group, Device* device) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&lora_state_event_subscriptions_mutex.handle);

    // Avoid cyclic subscription list that would loop forever
    for (LoraStateEventSubscription* existing = lora_state_event_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&lora_state_event_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->bit = bit;
    sub->internal.device = device;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = lora_state_event_subscriptions;
    lora_state_event_subscriptions = sub;
    mutex_unlock(&lora_state_event_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t lora_state_event_unsubscribe(LoraStateEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&lora_state_event_subscriptions_mutex.handle);
    for (LoraStateEventSubscription** link = &lora_state_event_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&lora_state_event_subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t lora_state_event_emit(Device* device, enum LoraRadioState state) {
    LoraStateEvent stamped_event { .timestamp = get_micros_since_boot(), .state = state };

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&lora_state_event_subscriptions_mutex.handle);
    for (LoraStateEventSubscription* sub = lora_state_event_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.device != device) {
            continue;
        }

        if (sub->internal.count >= LORA_STATE_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % LORA_STATE_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&lora_state_event_subscriptions_mutex.handle);

    return result;
}

error_t lora_state_event_poll(LoraStateEventSubscription* sub, LoraStateEvent* out_event) {
    mutex_lock(&lora_state_event_subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % LORA_STATE_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&lora_state_event_subscriptions_mutex.handle);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

// region RX events

error_t lora_rx_event_subscribe(LoraRxEventSubscription* sub, TaskEventGroup* event_group, Device* device) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&lora_rx_event_subscriptions_mutex.handle);

    // Avoid cyclic subscription list that would loop forever
    for (LoraRxEventSubscription* existing = lora_rx_event_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&lora_rx_event_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->bit = bit;
    sub->internal.device = device;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = lora_rx_event_subscriptions;
    lora_rx_event_subscriptions = sub;
    mutex_unlock(&lora_rx_event_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t lora_rx_event_unsubscribe(LoraRxEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&lora_rx_event_subscriptions_mutex.handle);
    for (LoraRxEventSubscription** link = &lora_rx_event_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&lora_rx_event_subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t lora_rx_event_emit(Device* device, const uint8_t* data, size_t length, float rssi, float snr) {
    LoraRxEvent stamped_event {};
    stamped_event.timestamp = get_micros_since_boot();
    stamped_event.length = std::min(length, sizeof(stamped_event.data));
    std::memcpy(stamped_event.data, data, stamped_event.length);
    stamped_event.rssi = rssi;
    stamped_event.snr = snr;

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&lora_rx_event_subscriptions_mutex.handle);
    for (LoraRxEventSubscription* sub = lora_rx_event_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.device != device) {
            continue;
        }

        if (sub->internal.count >= LORA_RX_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % LORA_RX_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&lora_rx_event_subscriptions_mutex.handle);

    return result;
}

error_t lora_rx_event_poll(LoraRxEventSubscription* sub, LoraRxEvent* out_event) {
    mutex_lock(&lora_rx_event_subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % LORA_RX_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&lora_rx_event_subscriptions_mutex.handle);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

// region TX events

error_t lora_tx_event_subscribe(LoraTxEventSubscription* sub, TaskEventGroup* event_group, Device* device) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&lora_tx_event_subscriptions_mutex.handle);

    // Avoid cyclic subscription list that would loop forever
    for (LoraTxEventSubscription* existing = lora_tx_event_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&lora_tx_event_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->bit = bit;
    sub->internal.device = device;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = lora_tx_event_subscriptions;
    lora_tx_event_subscriptions = sub;
    mutex_unlock(&lora_tx_event_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t lora_tx_event_unsubscribe(LoraTxEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&lora_tx_event_subscriptions_mutex.handle);
    for (LoraTxEventSubscription** link = &lora_tx_event_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&lora_tx_event_subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t lora_tx_event_emit(Device* device, LoraTxId id, enum LoraTransmissionState state) {
    LoraTxEvent stamped_event { .timestamp = get_micros_since_boot(), .id = id, .state = state };

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&lora_tx_event_subscriptions_mutex.handle);
    for (LoraTxEventSubscription* sub = lora_tx_event_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.device != device) {
            continue;
        }

        if (sub->internal.count >= LORA_TX_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % LORA_TX_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&lora_tx_event_subscriptions_mutex.handle);

    return result;
}

error_t lora_tx_event_poll(LoraTxEventSubscription* sub, LoraTxEvent* out_event) {
    mutex_lock(&lora_tx_event_subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % LORA_TX_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&lora_tx_event_subscriptions_mutex.handle);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

error_t lora_get_radio_state(struct Device* device, enum LoraRadioState* state) {
    return LORA_API(device)->get_radio_state(device, state);
}

error_t lora_set_enabled(struct Device* device, bool enabled) {
    return LORA_API(device)->set_enabled(device, enabled);
}

error_t lora_set_modulation(struct Device* device, enum LoraModulation modulation) {
    return LORA_API(device)->set_modulation(device, modulation);
}

error_t lora_get_modulation(struct Device* device, enum LoraModulation* modulation) {
    return LORA_API(device)->get_modulation(device, modulation);
}

bool lora_can_transmit(struct Device* device, enum LoraModulation modulation) {
    return LORA_API(device)->can_transmit(device, modulation);
}

bool lora_can_receive(struct Device* device, enum LoraModulation modulation) {
    return LORA_API(device)->can_receive(device, modulation);
}

error_t lora_set_parameter(struct Device* device, enum LoraParameter parameter, int32_t value) {
    return LORA_API(device)->set_parameter(device, parameter, value);
}

error_t lora_get_parameter(struct Device* device, enum LoraParameter parameter, int32_t* value) {
    return LORA_API(device)->get_parameter(device, parameter, value);
}

error_t lora_transmit(struct Device* device, const uint8_t* data, size_t length, LoraTxId* id) {
    return LORA_API(device)->transmit(device, data, length, id);
}

const struct DeviceType LORA_TYPE = {
    .name = "lora"
};

} // extern "C"
