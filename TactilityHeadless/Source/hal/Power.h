#pragma once

#include <cstdint>

namespace tt::hal {

class Power{

public:

    Power() = default;
    virtual ~Power() = default;

    enum MetricType {
        IS_CHARGING, // bool
        CURRENT, // int32_t, mAh
        BATTERY_VOLTAGE, // uint32_t, mV
        CHARGE_LEVEL, // uint8_t [0, 100]
    };

    union MetricData {
        int32_t valueAsInt32 = 0;
        uint32_t valueAsUint32;
        uint8_t valueAsUint8;
        float valueAsFloat;
        bool valueAsBool;
    };

    virtual bool supportsMetric(MetricType type) const = 0;

    /**
     * @return false when metric is not supported or (temporarily) not available.
     */
    virtual bool getMetric(Power::MetricType type, MetricData& data) = 0;

    virtual bool supportsChargeControl() const { return false; }
    virtual bool isAllowedToCharge() const { return false; }
    virtual void setAllowedToCharge(bool canCharge) { /* NO-OP*/ }
};

} // namespace tt
