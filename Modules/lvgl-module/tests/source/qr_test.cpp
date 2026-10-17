#include "doctest.h"

#include <lvgl.h>
#include <lvgl/widgets/qr.h>

#include <string>

TEST_CASE("lvgl_qr_set_data accepts data that fits in a QR code and rejects data that doesn't") {
    lv_init();
    lv_display_create(320, 240);
    lv_obj_t* qr = lvgl_qr_create(lv_screen_active());

    CHECK_EQ(lvgl_qr_set_data(qr, "hello"), ERROR_NONE);
    // Longer data needs a larger version
    CHECK_EQ(lvgl_qr_set_data(qr, std::string(17, 'a').c_str()), ERROR_NONE);
    CHECK_EQ(lvgl_qr_set_data(qr, std::string(18, 'a').c_str()), ERROR_NONE);
    CHECK_EQ(lvgl_qr_set_data(qr, std::string(1273, 'a').c_str()), ERROR_NONE);
    CHECK_EQ(lvgl_qr_set_data(qr, std::string(1274, 'a').c_str()), ERROR_OUT_OF_RANGE);
    CHECK_EQ(lvgl_qr_set_data(qr, std::string(5000, 'a').c_str()), ERROR_OUT_OF_RANGE);
    // A failure keeps the previous code
    CHECK_EQ(lvgl_qr_set_data(qr, ""), ERROR_NONE);

    lv_obj_delete(qr);
    lv_deinit();
}
