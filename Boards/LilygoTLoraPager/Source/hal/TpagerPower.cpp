#include "TpagerPower.h"

#include <Tactility/Log.h>

#define TAG "power"

#define TPAGER_GAUGE_I2C_BUS_HANDLE I2C_NUM_0

/*
TpagerPower::TpagerPower() : gauge(TPAGER_GAUGE_I2C_BUS_HANDLE) {
    gauge->configureCapacity(1500, 1500);
}*/

TpagerPower::~TpagerPower() {}

bool TpagerPower::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case IsCharging:
        case Current:
        case BatteryVoltage:
        case ChargeLevel:
            return true;
        default:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool TpagerPower::getMetric(MetricType type, MetricData& data) {
    /*    IsCharging, // bool
    Current, // int32_t, mAh - battery current: either during charging (positive value) or discharging (negative value)
    BatteryVoltage, // uint32_t, mV
    ChargeLevel, // uint8_t [0, 100]
*/

    uint16_t u16 = 0;
    int16_t s16 = 0;
    switch (type) {
        using enum MetricType;
        case IsCharging:
            Bq27220::BatteryStatus status;
            if (gauge->getBatteryStatus(status)) {
                data.valueAsBool = !status.reg.DSG;
                return true;
            }
            return false;
            break;
        case Current:
            if (gauge->getCurrent(s16)) {
                data.valueAsInt32 = s16;
                return true;
            } else {
                return false;
            }
            break;
        case BatteryVoltage:
            if (gauge->getVoltage(u16)) {
                data.valueAsUint32 = u16;
                return true;
            } else {
                return false;
            }
            break;
        case ChargeLevel:
            if (gauge->getStateOfCharge(u16)) {
                data.valueAsUint8 = u16;
                return true;
            } else {
                return false;
            }
            break;
        default:
            return false;
            break;
    }

    return false; // Safety guard for when new enum values are introduced
}

static std::shared_ptr<PowerDevice> power;
extern std::shared_ptr<Bq27220> bq27220;

std::shared_ptr<PowerDevice> tpager_get_power() {
    if (power == nullptr) {
        power = std::make_shared<TpagerPower>(bq27220);
    }
    return power;
}
