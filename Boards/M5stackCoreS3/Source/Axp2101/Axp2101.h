#pragma once

#include "hal/i2c/I2c.h"

/* AXP2101 usage
|           |M5Stack<BR>Core2v1.1  |M5Stack<BR>CoreS3<BR>CoreS3SE| |
|:---------:|:--------------------:|:-----------------:|:---------:|
| ALDO1     | ---                  |VDD 1v8            | ALDO1     |
| ALDO2     |LCD RST               |VDDA 3v3           | ALDO2     |
| ALDO3     |SPK EN                |CAM 3v3            | ALDO3     |
| ALDO4     |Periph PW<BR>TF,TP,LCD|TF 3v3             | ALDO4     |
| BLDO1     |LCD BL                |AVDD               | BLDO1     |
| BLDO2     |PORT 5V EN            |DVDD               | BLDO2     |
| DLDO1/DC1 |VIB MOTOR             |LCD BL             | DLDO1/DC1 |
| DLDO2/DC2 | ---                  | ---               | DLDO2/DC2 |
| BACKUP    |RTC BAT               |RTC BAT            | BACKUP    |
Source: https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/README.md?plain=1#L223
*/

/**
 * References:
 * - https://github.com/m5stack/M5Unified/blob/master/src/utility/AXP2101_Class.cpp
 * - http://file.whycan.com/files/members/6736/AXP2101_Datasheet_V1.0_en_3832.pdf
 *
 *
 */
class Axp2101 {

    i2c_port_t port;

    static constexpr uint8_t DEFAULT_ADDRESS = 0x34;
    static constexpr TickType_t DEFAULT_TIMEOUT = 1000 / portTICK_PERIOD_MS;

    bool readRegister8(uint8_t reg, uint8_t& result) const;
    bool writeRegister8(uint8_t reg, uint8_t value) const;
    bool readRegister12(uint8_t reg, float& out) const;
    bool readRegister14(uint8_t reg, float& out) const;
    bool readRegister16(uint8_t reg, uint16_t& out) const;

public:

    enum ChargeStatus {
        CHARGE_STATUS_CHARGING = 0b01,
        CHARGE_STATUS_DISCHARGING = 0b10,
        CHARGE_STATUS_STANDBY = 0b00
    };

    Axp2101(i2c_port_t port) : port(port) {}

    bool getBatteryVoltage(float& vbatMillis) const;
    bool getChargeStatus(ChargeStatus& status) const;
    bool isChargingEnabled(bool& enabled) const;
    bool setChargingEnabled(bool enabled) const;
};
