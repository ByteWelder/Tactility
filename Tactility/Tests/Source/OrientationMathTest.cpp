#include "doctest.h"
#include <Tactility/service/autorotate/OrientationMath.h>

using tt::service::autorotate::OrientationMath;

TEST_CASE("a flat reading below the threshold keeps the current rotation") {
    OrientationMath math;
    math.reset(LV_DISPLAY_ROTATION_0);
    CHECK_EQ(math.update(0.1F, 0.1F), LV_DISPLAY_ROTATION_0);
}

TEST_CASE("accel toward +X classifies as rotation 270 when no debounce is needed yet") {
    OrientationMath math;
    math.reset(LV_DISPLAY_ROTATION_0);
    // A single reading only sets the debounce target - the previous rotation is still returned.
    CHECK_EQ(math.update(1.0F, 0.0F), LV_DISPLAY_ROTATION_0);
}

TEST_CASE("a new rotation is only applied after enough consecutive stable readings") {
    OrientationMath math;
    math.reset(LV_DISPLAY_ROTATION_0);

    // The 1st call only sets the debounce target; 5 more consecutive agreeing calls are then needed.
    lv_display_rotation_t last = LV_DISPLAY_ROTATION_0;
    for (int i = 0; i < 5; i++) {
        last = math.update(1.0F, 0.0F);
        CHECK_EQ(last, LV_DISPLAY_ROTATION_0); // not yet debounced
    }
    last = math.update(1.0F, 0.0F);
    CHECK_EQ(last, LV_DISPLAY_ROTATION_270); // 6th call (5th consecutive stable reading) switches
}

TEST_CASE("all four accel directions map to the expected rotation") {
    OrientationMath math;
    math.reset(LV_DISPLAY_ROTATION_0);

    auto settle = [&](float ax, float ay) {
        lv_display_rotation_t result = LV_DISPLAY_ROTATION_0;
        for (int i = 0; i < 6; i++) {
            result = math.update(ax, ay);
        }
        return result;
    };

    // Accel points up (opposite gravity); UI top follows it.
    // LVGL ROT90 is CCW (top -> -X), ROT270 is CW (top -> +X).
    CHECK_EQ(settle(0.0F, 1.0F), LV_DISPLAY_ROTATION_0);
    CHECK_EQ(settle(1.0F, 0.0F), LV_DISPLAY_ROTATION_270);
    CHECK_EQ(settle(0.0F, -1.0F), LV_DISPLAY_ROTATION_180);
    CHECK_EQ(settle(-1.0F, 0.0F), LV_DISPLAY_ROTATION_90);
}

TEST_CASE("an unstable target resets the debounce counter") {
    OrientationMath math;
    math.reset(LV_DISPLAY_ROTATION_0);

    // Get close to switching to rotation 270...
    for (int i = 0; i < 4; i++) {
        math.update(1.0F, 0.0F);
    }
    // ...then a different target interrupts the debounce run.
    lv_display_rotation_t result = math.update(0.0F, -1.0F);
    CHECK_EQ(result, LV_DISPLAY_ROTATION_0);

    // Confirms the rotation-180 debounce restarted from zero rather than continuing.
    for (int i = 0; i < 4; i++) {
        result = math.update(0.0F, -1.0F);
        CHECK_EQ(result, LV_DISPLAY_ROTATION_0);
    }
    result = math.update(0.0F, -1.0F);
    CHECK_EQ(result, LV_DISPLAY_ROTATION_180);
}
