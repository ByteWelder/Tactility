// SPDX-License-Identifier: Apache-2.0
#include <location/gps_provider.h>

#include <gps/gps.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/device_listener.h>
#include <tactility/log.h>
#include <tactility/time.h>

#include <atomic>
#include <cmath>
#include <ctime>

#define TAG "location_gps"

namespace {

constexpr uint32_t AWAIT_TIMEOUT_MS = 250;
constexpr uint32_t DEVICE_POLL_INTERVAL_MS = 1000;
constexpr float METERS_PER_SECOND_PER_KNOT = 0.514444f;
// Typical user equivalent range error of consumer receivers, which turns dilution of precision into meters
constexpr float RANGE_ERROR_METERS = 5.0f;

/** The sentences received for one GPS epoch */
struct Epoch {
    minmea_time time;
    bool hasRmc;
    bool hasGga;
    bool reported;
    minmea_sentence_rmc rmc;
    minmea_sentence_gga gga;
};

struct GpsProvider {
    LocationHost* host = nullptr;
    Thread* thread = nullptr;
    std::atomic<bool> running = false;
    // Guards device and subscription. The device listener can outlive a provider run, so this state is static.
    Mutex mutex {};
    Device* device = nullptr;
    GpsSubscription subscription {};
    Epoch epoch {};

    GpsProvider() { mutex_construct(&mutex); }
    ~GpsProvider() { mutex_destruct(&mutex); }
};

GpsProvider provider;

double toDegrees(const minmea_float& value) {
    const double raw = static_cast<double>(value.value) / value.scale;
    const int degrees = static_cast<int>(raw / 100);
    return degrees + (raw - degrees * 100) / 60.0;
}

// Days from 1970-01-01 to a date in the proleptic Gregorian calendar
int64_t daysSinceEpoch(int year, int month, int day) {
    year -= month <= 2;
    const int64_t era = (year >= 0 ? year : year - 399) / 400;
    const int yearOfEra = static_cast<int>(year - era * 400);
    const int dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

bool isSameTime(const minmea_time& a, const minmea_time& b) {
    return a.hours == b.hours && a.minutes == b.minutes && a.seconds == b.seconds && a.microseconds == b.microseconds;
}

void report(Epoch& epoch) {
    Location location;
    if (gps_location_from_nmea(&epoch.rmc, epoch.hasGga ? &epoch.gga : nullptr, &location)) {
        if (location_get_precision() == LOCATION_PRECISION_APPROXIMATE) {
            location_make_approximate(&location);
        }
        location_host_report(provider.host, &location);
    }
    epoch.reported = true;
}

void onSentence(const GpsEvent& event) {
    auto& epoch = provider.epoch;
    const minmea_time& time = event.type == GPS_EVENT_MESSAGE_RMC ? event.data.rmc.time : event.data.gga.time;
    if (!isSameTime(epoch.time, time)) {
        // Receivers that don't send GGA only get reported once the next epoch starts
        if (epoch.hasRmc && !epoch.reported) {
            report(epoch);
        }
        epoch = {};
        epoch.time = time;
    }

    if (event.type == GPS_EVENT_MESSAGE_RMC) {
        epoch.rmc = event.data.rmc;
        epoch.hasRmc = true;
    } else {
        epoch.gga = event.data.gga;
        epoch.hasGga = true;
    }

    if (epoch.hasRmc && epoch.hasGga && !epoch.reported) {
        report(epoch);
    }
}

// Unsubscribes before the driver stops, as the subscription can't be removed afterwards
void onDeviceEvent(Device* device, DeviceEvent event, void*) {
    if (event != DEVICE_EVENT_STOPPING) {
        return;
    }
    mutex_lock(&provider.mutex);
    if (device == provider.device) {
        gps_event_unsubscribe(device, &provider.subscription);
        provider.device = nullptr;
    }
    mutex_unlock(&provider.mutex);
}

/** Must be called with the mutex held */
bool subscribe() {
    Device* device;
    if (device_get_first_active_by_type(&GPS_TYPE, &device) != ERROR_NONE) {
        return false;
    }
    const error_t error = gps_event_subscribe(device, &provider.subscription);
    device_put(device);
    if (error != ERROR_NONE) {
        LOG_E(TAG, "Failed to subscribe to %s", device->name);
        return false;
    }
    LOG_I(TAG, "Using %s", device->name);
    provider.device = device;
    provider.epoch = {};
    return true;
}

/** Must be called with the mutex held and a device subscribed */
void receive() {
    auto& sub = provider.subscription;
    if (sub.sequence != sub.consumed_sequence) {
        sub.consumed_sequence = sub.sequence;
    } else if (gps_event_await(provider.device, &sub, millis_to_ticks(AWAIT_TIMEOUT_MS)) != ERROR_NONE) {
        return;
    }

    switch (sub.event.type) {
        case GPS_EVENT_MESSAGE_RMC:
        case GPS_EVENT_MESSAGE_GGA:
            onSentence(sub.event);
            break;
        case GPS_EVENT_UNSUBSCRIBED:
            provider.device = nullptr;
            break;
    }
}

int32_t threadMain(void*) {
    while (provider.running) {
        mutex_lock(&provider.mutex);
        const bool subscribed = provider.device != nullptr || subscribe();
        if (subscribed) {
            receive();
        }
        mutex_unlock(&provider.mutex);

        if (!subscribed) {
            delay_millis(DEVICE_POLL_INTERVAL_MS);
        }
    }

    mutex_lock(&provider.mutex);
    if (provider.device != nullptr) {
        gps_event_unsubscribe(provider.device, &provider.subscription);
        provider.device = nullptr;
    }
    mutex_unlock(&provider.mutex);
    return 0;
}

void* create(LocationHost* host) {
    provider.host = host;
    provider.running = true;
    provider.thread = thread_alloc_full("location_gps", 4096, threadMain, nullptr, -1);
    if (provider.thread == nullptr) {
        return nullptr;
    }
    // Settings are loaded before providers start, so the thread doesn't access flash
    thread_set_stack_external(provider.thread, true);
    device_listener_add(onDeviceEvent, nullptr);
    if (thread_start(provider.thread) != ERROR_NONE) {
        device_listener_remove(onDeviceEvent, nullptr);
        thread_free(provider.thread);
        provider.thread = nullptr;
        return nullptr;
    }
    return &provider;
}

void destroy(void*) {
    provider.running = false;
    // The provider state can only be reused after the worker exits. It does so within AWAIT_TIMEOUT_MS.
    thread_join(provider.thread, portMAX_DELAY, millis_to_ticks(10));
    device_listener_remove(onDeviceEvent, nullptr);
    thread_free(provider.thread);
    provider.thread = nullptr;
}

} // namespace

bool gps_location_from_nmea(const minmea_sentence_rmc* rmc, const minmea_sentence_gga* gga, Location* location) {
    if (!rmc->valid || rmc->latitude.scale == 0 || rmc->longitude.scale == 0) {
        return false;
    }

    *location = Location {
        .latitude = toDegrees(rmc->latitude),
        .longitude = toDegrees(rmc->longitude),
        .horizontal_accuracy = NAN,
        .altitude = NAN,
        .ellipsoidal_altitude = NAN,
        .vertical_accuracy = NAN,
        .speed = minmea_tofloat(&rmc->speed) * METERS_PER_SECOND_PER_KNOT,
        .speed_accuracy = NAN,
        .course = minmea_tofloat(&rmc->course),
        .course_accuracy = NAN,
        .satellites = 0,
        .time_ms = 0,
        .uptime_ms = 0,
        .provider = {},
    };

    tm dateTime;
    if (minmea_getdatetime(&dateTime, &rmc->date, &rmc->time) == 0) {
        const int64_t seconds = daysSinceEpoch(dateTime.tm_year + 1900, dateTime.tm_mon + 1, dateTime.tm_mday) * 86400 +
            dateTime.tm_hour * 3600 + dateTime.tm_min * 60 + dateTime.tm_sec;
        location->time_ms = seconds * 1000 + rmc->time.microseconds / 1000;
    }

    if (gga != nullptr && gga->fix_quality > 0) {
        location->horizontal_accuracy = minmea_tofloat(&gga->hdop) * RANGE_ERROR_METERS;
        location->altitude = minmea_tofloat(&gga->altitude);
        location->ellipsoidal_altitude = location->altitude + minmea_tofloat(&gga->height);
        location->satellites = static_cast<uint8_t>(gga->satellites_tracked);
    }

    return true;
}

const LocationProviderApi gps_location_provider = {
    .id = "gps",
    .fallback = false,
    .create = create,
    .destroy = destroy,
};
