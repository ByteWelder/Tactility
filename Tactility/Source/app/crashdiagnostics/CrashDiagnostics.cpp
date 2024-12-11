#include <sstream>
#include <esp_cpu_utils.h>
#include "lvgl.h"
#include "lvgl/Statusbar.h"
#include "kernel/PanicHandler.h"
#include "service/loader/Loader.h"
#include "qrcode.h"

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

    lv_obj_add_event_cb(parent, onContinuePressed, LV_EVENT_CLICKED, nullptr);

    auto* canvas = lv_canvas_create(parent);
    lv_obj_set_size(canvas, LV_PCT(100), parent_height - button_height - button_margin);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_obj_set_content_height(canvas, 200);
    lv_obj_set_content_width(canvas, 200);

    auto* top_label = lv_label_create(parent);
    lv_label_set_text(top_label, "Oops! We've crashed ..."); // TODO: Funny messages
    lv_obj_align(top_label, LV_ALIGN_TOP_MID, 0, 2);

    auto* bottom_label = lv_label_create(parent);
    lv_label_set_text(bottom_label, "Tap screen to continue");
    lv_obj_align(bottom_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    auto* crash_data = getRtcCrashData();
    auto* stack_buffer = (uint32_t*)malloc(crash_data->callstackLength * 2 * sizeof(uint32_t));
    for (int i = 0; i < crash_data->callstackLength; ++i) {
        const CallstackFrame&frame = crash_data->callstack[i];
        uint32_t pc = esp_cpu_process_stack_pc(frame.pc);
        uint32_t sp = frame.sp;
        stack_buffer[i * 2] = pc;
        stack_buffer[(i * 2) + 1] = sp;
    }

    std::stringstream stream;

    stream << "https://oops.bytewelder.com?";
    stream << "i=1"; // Application id
    // stream << "&v=snapshot"; // Version
    stream << "&a=" << CONFIG_IDF_TARGET; // Architecture
    stream << "&s="; // Stacktrace

    for (int i = 0; i < crash_data->callstackLength; ++i) {
        uint32_t pc = stack_buffer[(i * 2)];
        uint32_t sp = stack_buffer[(i * 2) + 1];
        stream << std::hex << pc << std::hex << sp;
        TT_LOG_D(TAG, "%#08x:%#08x", (unsigned int)pc, (unsigned int)sp);
    }

    free(stack_buffer);

    std::string url = stream.str();
    size_t url_length = url.length();
    TT_LOG_I(TAG, "%s", url.c_str());
    // See http://blog.qr4.nl/page/QR-Code-Data-Capacity.aspx
    int qr_version;
    if (url_length <= 134) {
        qr_version = 6;
    } else if (url_length <= 192) {
        qr_version = 8;
    } else if (url_length <= 271) {
        qr_version = 10;
    } else if (url_length <= 367) {
        qr_version = 12;
    } else if (url_length <= 458) {
        qr_version = 14;
    } else if (url_length <= 586) {
        qr_version = 16;
    } else if (url_length <= 718) {
        qr_version = 18;
    } else if (url_length <= 858) {
        qr_version = 20;
    } else if (url_length <= 1003) {
        qr_version = 22;
    } else {
        // QR codes can be bigger, but they won't fit the screen
        TT_LOG_E(TAG, "Data too long to generate QR: %d", url_length);
        return;
    }
    TT_LOG_I(TAG, "QR version %d (length: %d)", qr_version, url_length);
    auto* qrcodeData = (uint8_t*)malloc(qrcode_getBufferSize(qr_version));

    if (qrcodeData != nullptr) {
        QRCode qrcode;
        TT_LOG_I(TAG, "QR init text");
        if (qrcode_initText(&qrcode, qrcodeData, qr_version, ECC_LOW, url.c_str()) == 0) {
            TT_LOG_I(TAG, "QR size: %d", qrcode.size);
            TT_LOG_I(TAG, "Create draw buffer");
            auto* draw_buf = lv_draw_buf_create(2 * qrcode.size, 2 * qrcode.size, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
            if (draw_buf != nullptr) {
                lv_canvas_set_draw_buf(canvas, draw_buf);

                for (uint8_t y = 0; y < qrcode.size; y++) {
                    for (uint8_t x = 0; x < qrcode.size; x++) {
                        bool colored = qrcode_getModule(&qrcode, x, y);
                        auto color = colored ? lv_color_white() : lv_color_black();
                        int32_t pos_x = x * 2;
                        int32_t pos_y = y * 2;
                        lv_canvas_set_px(canvas, pos_x, pos_y, color, LV_OPA_COVER);
                        lv_canvas_set_px(canvas, pos_x + 1, pos_y, color, LV_OPA_COVER);
                        lv_canvas_set_px(canvas, pos_x + 1, pos_y +1, color, LV_OPA_COVER);
                        lv_canvas_set_px(canvas, pos_x, pos_y +1, color, LV_OPA_COVER);
                    }
                }
            } else {
                TT_LOG_E(TAG, "Draw buffer alloc");
            }
        } else {
            TT_LOG_E(TAG, "QR alloc");
        }

        free(qrcodeData);
    }
}

extern const AppManifest manifest = {
    .id = "CrashDiagnostics",
    .name = "Crash Diagnostics",
    .type = TypeHidden,
    .onShow = onShow
};

} // namespace
