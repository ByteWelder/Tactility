#include <Check.h>
#include "WifiManage.h"

namespace tt::app::wifimanage {

void State::setScanning(bool isScanning) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    scanning = isScanning;
    tt_check(mutex.release() == TtStatusOk);
}

void State::setRadioState(service::wifi::WifiRadioState state) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    radioState = state;
    tt_check(mutex.release() == TtStatusOk);
}

void State::updateApRecords() {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    apRecords = service::wifi::getScanResults();
    tt_check(mutex.release() == TtStatusOk);
}

} // namespace
