#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if (defined(CONFIG_SOC_WIFI_SUPPORTED) || defined(CONFIG_ESP_HOSTED_ENABLED))

#include <Tactility/service/espnow/EspNow.h>
#include <Tactility/service/espnow/EspNowRadio.h>

#include <TactilityCpp/Memory.h>
#include <TactilityCpp/std/deque.h>
#include <TactilityCpp/std/vector.h>

#include <tactility/concurrent/event_group.h>
#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/radio.h>
#include <tactility/log.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <esp_wifi.h>

namespace tt::service::espnow {

constexpr auto* TAG = "EspNowRadio";

namespace {

constexpr uint32_t TX_BIT = (1 << 0);
constexpr uint32_t STOP_BIT = (1 << 1);
constexpr int32_t CHANNEL_MIN = 1;
constexpr int32_t CHANNEL_MAX = 14;
constexpr int32_t POWER_MIN = 2;
constexpr int32_t POWER_MAX = 20;
constexpr uint32_t ENABLE_TIMEOUT_MS = 5000;
constexpr uint8_t BROADCAST_ADDRESS[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

struct Frame {
    RadioTxId id;
    tt::std::vector<uint8_t> data;
};

struct Radio {
    Device* device = nullptr;
    Mutex mutex {};
    EventGroupHandle_t events = nullptr;
    Thread* thread = nullptr;
    RadioState state = RADIO_STATE_OFF;
    RadioModulation modulation = RADIO_MODULATION_NONE;
    /** Guarded by the mutex */
    int32_t channel = 1;
    int32_t power = POWER_MAX;
    RadioTxId nextTxId = 1;
    tt::std::deque<Frame> queue;
    ReceiverSubscription subscription = NO_SUBSCRIPTION;
    bool stopRequested = false;
};

Radio* getRadio(Device* device) {
    return static_cast<Radio*>(device_get_driver_data(device));
}

void setState(Radio* radio, RadioState state) {
    mutex_lock(&radio->mutex);
    radio->state = state;
    mutex_unlock(&radio->mutex);
    radio_state_event_emit(radio->device, state);
}

bool isStopRequested(Radio* radio) {
    mutex_lock(&radio->mutex);
    const bool stop = radio->stopRequested;
    mutex_unlock(&radio->mutex);
    return stop;
}

/** Waits until the ESP-NOW service has brought ESP-NOW up. The service does that on another task. */
bool waitUntilEnabled(Radio* radio) {
    for (uint32_t waited = 0; waited < ENABLE_TIMEOUT_MS; waited += 20) {
        if (isEnabled()) {
            return true;
        }
        if (isStopRequested(radio)) {
            return false;
        }
        delay_millis(20);
    }
    return isEnabled();
}

void onReceive(Radio* radio, const esp_now_recv_info_t* info, const uint8_t* data, int length) {
    if (length <= 0) {
        return;
    }
    const float rssi = (info != nullptr && info->rx_ctrl != nullptr) ? (float)info->rx_ctrl->rssi : 0.0f;
    // Wi-Fi doesn't report the signal-to-noise ratio
    radio_rx_event_emit(radio->device, data, ::std::min<size_t>((size_t)length, RADIO_RX_MAX_PACKET_LENGTH), rssi, NAN);
}

void transmitQueued(Radio* radio) {
    while (!isStopRequested(radio)) {
        mutex_lock(&radio->mutex);
        if (radio->queue.empty()) {
            mutex_unlock(&radio->mutex);
            return;
        }
        Frame frame = ::std::move(radio->queue.front());
        radio->queue.pop_front();
        mutex_unlock(&radio->mutex);

        radio_tx_event_emit(radio->device, frame.id, RADIO_TRANSMISSION_STATE_TRANSMIT_PENDING);
        const bool fits = frame.data.size() <= getMaxDataLength();
        // The frame is handed to the Wi-Fi stack, which doesn't report when it's out
        const bool sent = fits && send(BROADCAST_ADDRESS, frame.data.data(), frame.data.size());
        if (!sent) {
            LOG_W(TAG, "Failed to send %u bytes (the maximum is %u)", (unsigned)frame.data.size(), (unsigned)getMaxDataLength());
        }
        radio_tx_event_emit(radio->device, frame.id, sent ? RADIO_TRANSMISSION_STATE_TRANSMITTED : RADIO_TRANSMISSION_STATE_ERROR);
    }
}

int32_t radioThread(void* context) {
    auto* radio = static_cast<Radio*>(context);

    mutex_lock(&radio->mutex);
    const int32_t channel = radio->channel;
    const int32_t power = radio->power;
    mutex_unlock(&radio->mutex);

    // The same settings as MeshCore's ESP-NOW transport: a station with the long range protocol, unencrypted broadcasts
    uint8_t key[ESP_NOW_KEY_LEN] = {};
    enable(EspNowConfig(key, Mode::Station, (uint8_t)channel, true, false));

    if (!waitUntilEnabled(radio)) {
        if (!isStopRequested(radio)) {
            LOG_E(TAG, "ESP-NOW didn't start");
            setState(radio, RADIO_STATE_ERROR);
            return 1;
        }
        disable();
        setState(radio, RADIO_STATE_OFF);
        return 0;
    }
#ifdef CONFIG_SOC_WIFI_SUPPORTED
    // In units of 0.25 dBm
    esp_wifi_set_max_tx_power((int8_t)(power * 4));
#endif
    radio->subscription = subscribeReceiver([radio](const esp_now_recv_info_t* info, const uint8_t* data, int length) {
        onReceive(radio, info, data, length);
    });
    setState(radio, RADIO_STATE_ON);

    while (!isStopRequested(radio)) {
        uint32_t flags = 0;
        event_group_wait(radio->events, TX_BIT | STOP_BIT, false, true, &flags, portMAX_DELAY);
        transmitQueued(radio);
    }

    setState(radio, RADIO_STATE_OFF_PENDING);
    unsubscribeReceiver(radio->subscription);
    radio->subscription = NO_SUBSCRIPTION;
    disable();
    setState(radio, RADIO_STATE_OFF);
    return 0;
}

// region RadioApi

error_t apiGetState(Device* device, RadioState* state) {
    auto* radio = getRadio(device);
    mutex_lock(&radio->mutex);
    *state = radio->state;
    mutex_unlock(&radio->mutex);
    return ERROR_NONE;
}

void stopThread(Radio* radio) {
    if (radio->thread == nullptr) {
        return;
    }
    mutex_lock(&radio->mutex);
    radio->stopRequested = true;
    mutex_unlock(&radio->mutex);
    event_group_set(radio->events, STOP_BIT);
    thread_join(radio->thread, portMAX_DELAY, pdMS_TO_TICKS(10));
    thread_free(radio->thread);
    radio->thread = nullptr;
}

error_t apiSetEnabled(Device* device, bool enabled) {
    auto* radio = getRadio(device);
    if (enabled) {
        if (radio->modulation != RADIO_MODULATION_ESPNOW) {
            return ERROR_INVALID_STATE;
        }
        // A thread that ended by itself, e.g. after ESP-NOW failed to start, is replaced
        RadioState state;
        apiGetState(device, &state);
        if (radio->thread != nullptr && (state == RADIO_STATE_ON || state == RADIO_STATE_ON_PENDING)) {
            return ERROR_NONE;
        }
        stopThread(radio);

        mutex_lock(&radio->mutex);
        radio->stopRequested = false;
        radio->queue.clear();
        mutex_unlock(&radio->mutex);
        event_group_clear(radio->events, TX_BIT | STOP_BIT);
        setState(radio, RADIO_STATE_ON_PENDING);
        radio->thread = thread_alloc_full("espnow_radio", 4096, radioThread, radio, -1);
        if (radio->thread == nullptr || thread_start(radio->thread) != ERROR_NONE) {
            if (radio->thread != nullptr) {
                thread_free(radio->thread);
                radio->thread = nullptr;
            }
            setState(radio, RADIO_STATE_ERROR);
            return ERROR_RESOURCE;
        }
        return ERROR_NONE;
    }

    stopThread(radio);
    return ERROR_NONE;
}

error_t apiSetModulation(Device* device, RadioModulation modulation) {
    auto* radio = getRadio(device);
    if (modulation != RADIO_MODULATION_ESPNOW) {
        return ERROR_NOT_SUPPORTED;
    }
    RadioState state;
    apiGetState(device, &state);
    if (state == RADIO_STATE_ON || state == RADIO_STATE_ON_PENDING) {
        return ERROR_INVALID_STATE;
    }
    radio->modulation = modulation;
    return ERROR_NONE;
}

error_t apiGetModulation(Device* device, RadioModulation* modulation) {
    *modulation = getRadio(device)->modulation;
    return ERROR_NONE;
}

bool apiCanTransmitOrReceive(Device*, RadioModulation modulation) {
    return modulation == RADIO_MODULATION_ESPNOW;
}

error_t apiSetParameter(Device* device, RadioParameter parameter, int32_t value) {
    auto* radio = getRadio(device);
    switch (parameter) {
        case RADIO_PARAMETER_CHANNEL:
            if (value < CHANNEL_MIN || value > CHANNEL_MAX) {
                return ERROR_OUT_OF_RANGE;
            }
            mutex_lock(&radio->mutex);
            radio->channel = value;
            mutex_unlock(&radio->mutex);
            return ERROR_NONE;
        case RADIO_PARAMETER_POWER:
            if (value < POWER_MIN || value > POWER_MAX) {
                return ERROR_OUT_OF_RANGE;
            }
            mutex_lock(&radio->mutex);
            radio->power = value;
            mutex_unlock(&radio->mutex);
            return ERROR_NONE;
        default:
            return ERROR_NOT_SUPPORTED;
    }
}

error_t apiGetParameter(Device* device, RadioParameter parameter, int32_t* value) {
    auto* radio = getRadio(device);
    mutex_lock(&radio->mutex);
    error_t result = ERROR_NONE;
    if (parameter == RADIO_PARAMETER_CHANNEL) {
        *value = radio->channel;
    } else if (parameter == RADIO_PARAMETER_POWER) {
        *value = radio->power;
    } else {
        result = ERROR_NOT_SUPPORTED;
    }
    mutex_unlock(&radio->mutex);
    return result;
}

error_t apiTransmit(Device* device, const uint8_t* data, size_t length, RadioTxId* id) {
    auto* radio = getRadio(device);
    mutex_lock(&radio->mutex);
    if (radio->state != RADIO_STATE_ON && radio->state != RADIO_STATE_ON_PENDING) {
        mutex_unlock(&radio->mutex);
        return ERROR_INVALID_STATE;
    }
    const RadioTxId txId = radio->nextTxId++;
    radio->queue.push_back(Frame { .id = txId, .data = tt::std::vector<uint8_t>(data, data + length) });
    mutex_unlock(&radio->mutex);

    if (id != nullptr) {
        *id = txId;
    }
    radio_tx_event_emit(device, txId, RADIO_TRANSMISSION_STATE_QUEUED);
    event_group_set(radio->events, TX_BIT);
    return ERROR_NONE;
}

const RadioApi radioApi = {
    .get_radio_state = apiGetState,
    .set_enabled = apiSetEnabled,
    .set_modulation = apiSetModulation,
    .get_modulation = apiGetModulation,
    .can_transmit = apiCanTransmitOrReceive,
    .can_receive = apiCanTransmitOrReceive,
    .set_parameter = apiSetParameter,
    .get_parameter = apiGetParameter,
    .transmit = apiTransmit,
    .get_rssi = nullptr,
    .is_receiving = nullptr,
};

// endregion

error_t startDevice(Device* device) {
    auto* radio = tt::newObject<Radio>();
    if (radio == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }
    radio->device = device;
    mutex_construct(&radio->mutex);
    event_group_construct(&radio->events);
    device_set_driver_data(device, radio);
    return ERROR_NONE;
}

error_t stopDevice(Device* device) {
    auto* radio = getRadio(device);
    stopThread(radio);
    device_set_driver_data(device, nullptr);
    event_group_destruct(&radio->events);
    mutex_destruct(&radio->mutex);
    tt::deleteObject(radio);
    return ERROR_NONE;
}

Driver driver = {
    .name = "espnow_radio",
    .compatible = (const char*[]) { "tactility,espnow-radio", nullptr },
    .start_device = startDevice,
    .stop_device = stopDevice,
    .probe = nullptr,
    .api = &radioApi,
    .device_type = &RADIO_TYPE,
    .internal = nullptr,
};

Device device {
    .address = 0,
    .name = "espnow-radio",
    .config = nullptr,
    .parent = nullptr,
    .flags = 0,
    .internal = nullptr,
};

} // namespace

void registerRadio() {
    if (driver_construct_add(&driver) != ERROR_NONE ||
        device_construct_add(&device, "tactility,espnow-radio") != ERROR_NONE ||
        device_start(&device) != ERROR_NONE) {
        LOG_E(TAG, "Failed to add the ESP-NOW radio");
    }
}

}

#endif
