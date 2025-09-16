#include <PwmBacklight.h>
#include <Tactility/Log.h>

constexpr auto* TAG = "Cardputer";

bool initBoot() {
    TT_LOG_I(TAG, "initBoot");

    return driver::pwmbacklight::init(GPIO_NUM_38, 512);
}