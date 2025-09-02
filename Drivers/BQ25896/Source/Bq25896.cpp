#include "Bq25896.h"

#define TAG "BQ27220"

void Bq25896::powerOff() {
    TT_LOG_I(TAG, "Power off");
    bitOn(0x09, BIT(5));
}

void Bq25896::powerOn() {
    TT_LOG_I(TAG, "Power on");
    bitOff(0x09, BIT(5));
}
