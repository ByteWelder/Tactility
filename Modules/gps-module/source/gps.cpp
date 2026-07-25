#include <gps/gps.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* gps_model_to_string(GpsModel model) {
    switch (model) {
        case GPS_MODEL_AG3335: return "AG3335";
        case GPS_MODEL_AG3352: return "AG3352";
        case GPS_MODEL_ATGM336H: return "ATGM336H";
        case GPS_MODEL_LS20031: return "LS20031";
        case GPS_MODEL_MTK: return "MTK";
        case GPS_MODEL_MTK_L76B: return "MTK_L76B";
        case GPS_MODEL_MTK_PA1616S: return "MTK_PA1616S";
        case GPS_MODEL_UBLOX6: return "UBLOX6";
        case GPS_MODEL_UBLOX7: return "UBLOX7";
        case GPS_MODEL_UBLOX8: return "UBLOX8";
        case GPS_MODEL_UBLOX9: return "UBLOX9";
        case GPS_MODEL_UBLOX10: return "UBLOX10";
        case GPS_MODEL_UC6580: return "UC6580";
        default: return "Unknown";
    }
}

error_t gps_event_subscribe(Device* device, GpsSubscription* sub) {
    const auto* driver = device_get_driver(device);
    return static_cast<const GpsApi*>(driver->api)->event_subscribe(device, sub);
}

error_t gps_event_unsubscribe(Device* device, GpsSubscription* sub) {
    const auto* driver = device_get_driver(device);
    return static_cast<const GpsApi*>(driver->api)->event_unsubscribe(device, sub);
}

error_t gps_event_await(Device* device, GpsSubscription* sub, TickType_t timeout) {
    const auto* driver = device_get_driver(device);
    return static_cast<const GpsApi*>(driver->api)->event_await(device, sub, timeout);
}

GpsState gps_get_state(Device* device) {
    const auto* driver = device_get_driver(device);
    return static_cast<const GpsApi*>(driver->api)->get_state(device);
}

error_t gps_get_model_name(Device* device, char* model_name, size_t buffer_size) {
    const auto* driver = device_get_driver(device);
    return static_cast<const GpsApi*>(driver->api)->get_model_name(device, model_name, buffer_size);
}

const DeviceType GPS_TYPE {
    .name = "gps"
};

#ifdef __cplusplus
}
#endif
