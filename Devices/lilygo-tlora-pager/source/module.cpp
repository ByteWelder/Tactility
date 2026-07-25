#include <lvgl/lvgl.h>
#include <tactility/module.h>

#include <Tactility/SystemEvents.h>
#include <Tactility/kernel/Kernel.h>

#include <lilygo/drivers/tpager_encoder_input.h>

constexpr auto* TAG = "T-Lora Pager";

extern "C" {

tt::kernel::SystemEventSubscription event_subscription = tt::kernel::NoSystemEventSubscription;

static error_t start() {
    event_subscription = tt::kernel::subscribeSystemEvent(tt::kernel::SystemEvent::BootSplash, [](tt::kernel::SystemEvent) {
        // The kernel tpager_encoder device is already started by kernel_init(); this just
        // registers it as an LVGL input device, which requires LVGL to be up first.
        lvgl_lock();
        tpager_encoder::init();
        lvgl_unlock();
    });
    return ERROR_NONE;
}

static error_t stop() {
    tt::kernel::unsubscribeSystemEvent(event_subscription);
    event_subscription = tt::kernel::NoSystemEventSubscription;
    return ERROR_NONE;
}

Module lilygo_tlora_pager_module = {
    .name = "lilygo-tlora-pager",
    .start = start,
    .stop = stop
};

}
