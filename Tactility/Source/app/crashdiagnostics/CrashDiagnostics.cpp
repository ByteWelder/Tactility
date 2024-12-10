#include <esp_debug_helpers.h>
#include <sstream>
#include <esp_cpu_utils.h>
#include "lvgl.h"
#include "lvgl/Statusbar.h"
#include "kernel/PanicHandler.h"
#include "service/loader/Loader.h"

#define TAG "crash_diagnostics"

namespace tt::app::crashdiagnostics {

void onContinuePressed(TT_UNUSED lv_event_t* event) {
    tt::service::loader::stopApp();
    tt::service::loader::startApp("Desktop");
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto* display = lv_obj_get_display(parent);
    int32_t parent_height = lv_display_get_vertical_resolution(display) - STATUSBAR_HEIGHT;

    int32_t button_height = 40;
    int32_t button_margin = 16;
    auto* message_wrapper = lv_obj_create(parent);
    lv_obj_align(message_wrapper, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(message_wrapper, LV_PCT(100), parent_height - button_height - button_margin);

    auto* message = lv_label_create(message_wrapper);
    lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
    lv_obj_set_size(message, LV_PCT(100), LV_SIZE_CONTENT);

    auto* button = lv_button_create(parent);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(button, onContinuePressed, LV_EVENT_CLICKED, nullptr);
    auto* button_label = lv_label_create(button);
    lv_label_set_text(button_label, "Continue");

    const CrashData* data = getRtcCrashData();
    std::stringstream stream;
    for (int i = 0; i < data->callstackLength; ++i) {
        const CallstackFrame& frame = data->callstack[i];
        stream << std::hex << esp_cpu_process_stack_pc(frame.pc) << ':' << std::hex << frame.sp << ' ';
    }
    stream << std::endl;

    lv_label_set_text(message, stream.str().c_str());
}

extern const AppManifest manifest = {
    .id = "CrashDiagnostics",
    .name = "Crash Diagnostics",
    .type = TypeHidden,
    .onShow = onShow
};

} // namespace
