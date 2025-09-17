#include <Tactility/service/loader/Loader.h>
#include <Tactility/Assets.h>
#include <Tactility/app/gpio/GpioHal.h>
#include "Tactility/lvgl/Toolbar.h"
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/lvgl/Color.h>
#include <Tactility/Mutex.h>
#include <Tactility/Timer.h>

namespace tt::app::gpio {

extern const AppManifest manifest;

class GpioApp : public App {

    lv_obj_t* lvPins[GPIO_NUM_MAX] = { nullptr };
    uint8_t pinStates[GPIO_NUM_MAX] = { 0 };
    std::unique_ptr<Timer> timer;
    Mutex mutex;

    static lv_obj_t* createGpioRowWrapper(lv_obj_t* parent);
    void onTimer();

public:

    void onShow(AppContext& app, lv_obj_t* parent) override;
    void onHide(AppContext& app) override;

    void startTask();
    void stopTask();

    void updatePinStates();
    void updatePinWidgets();
};

void GpioApp::updatePinStates() {
    mutex.lock();
    // Update pin states
    for (int i = 0; i < GPIO_NUM_MAX; ++i) {
#ifdef ESP_PLATFORM
        pinStates[i] = gpio_get_level(static_cast<gpio_num_t>(i));
#else
        pinStates[i] = gpio_get_level(i);
#endif
    }
    mutex.unlock();
}

void GpioApp::updatePinWidgets() {
    auto scoped_lvgl_lock = lvgl::getSyncLock()->asScopedLock();
    auto scoped_gpio_lock = mutex.asScopedLock();
    if (scoped_gpio_lock.lock() && scoped_lvgl_lock.lock(lvgl::defaultLockTime)) {
        for (int j = 0; j < GPIO_NUM_MAX; ++j) {
            int level = pinStates[j];
            lv_obj_t* label = lvPins[j];
            void* label_user_data = lv_obj_get_user_data(label);
            // The user data stores the state, so we can avoid unnecessary updates
            if (reinterpret_cast<void*>(level) != label_user_data) {
                lv_obj_set_user_data(label, reinterpret_cast<void*>(level));
                if (level == 0) {
                    lv_obj_set_style_text_color(label, lv_color_background_darkest(), LV_STATE_DEFAULT);
                } else {
                    lv_obj_set_style_text_color(label, lv_color_make(0, 200, 0), LV_STATE_DEFAULT);
                }
            }
        }
    }
}

lv_obj_t* GpioApp::createGpioRowWrapper(lv_obj_t* parent) {
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_style_pad_all(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    return wrapper;
}

// region Task

void GpioApp::onTimer() {
    updatePinStates();
    updatePinWidgets();
}

void GpioApp::startTask() {
    mutex.lock();
    assert(timer == nullptr);
    timer = std::make_unique<Timer>(Timer::Type::Periodic, [this] {
        onTimer();
    });
    timer->start(100 / portTICK_PERIOD_MS);
    mutex.unlock();
}

void GpioApp::stopTask() {
    assert(timer);

    timer->stop();
    timer = nullptr;
}

// endregion Task

static int getSquareSpacing(hal::UiScale uiScale) {
    if (uiScale == hal::UiScale::Smallest) {
        return 0;
    } else {
        return 4;
    }
}

void GpioApp::onShow(AppContext& app, lv_obj_t* parent) {
    auto ui_scale = hal::getConfiguration()->uiScale;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    auto* toolbar = lvgl::toolbar_create(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    // Main content wrapper, enables scrolling content without scrolling the toolbar
    auto* expansion_wrapper = lv_obj_create(parent);
    lv_obj_set_width(expansion_wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(expansion_wrapper, 1);
    lv_obj_set_style_border_width(expansion_wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(expansion_wrapper, 0, LV_STATE_DEFAULT);

    auto* centering_wrapper = lv_obj_create(expansion_wrapper);
    lv_obj_set_size(centering_wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(centering_wrapper, LV_ALIGN_CENTER);
    lv_obj_set_style_border_width(centering_wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(centering_wrapper, 0, LV_STATE_DEFAULT);

    auto* display = lv_obj_get_display(parent);
    auto horizontal_px = lv_display_get_horizontal_resolution(display);
    auto vertical_px = lv_display_get_vertical_resolution(display);
    bool is_landscape_display = horizontal_px > vertical_px;

    constexpr auto block_width = 16;
    const auto square_spacing = getSquareSpacing(ui_scale);
    int32_t x_spacing = block_width + square_spacing;
    uint8_t column = 0;
    const uint8_t column_limit = is_landscape_display ? 10 : 5;

    auto* row_wrapper = createGpioRowWrapper(centering_wrapper);
    lv_obj_align(row_wrapper, LV_ALIGN_TOP_MID, 0, 0);

    mutex.lock();
    for (int i = GPIO_NUM_MIN; i < GPIO_NUM_MAX; ++i) {
        constexpr uint8_t offset_from_left_label = 4;

        // Add the GPIO number before the first item on a row
        if (column == 0) {
            auto* prefix = lv_label_create(row_wrapper);
            lv_label_set_text_fmt(prefix, "%02d", i);
        }

        // Add a new GPIO status indicator
        auto* status_label = lv_label_create(row_wrapper);
        lv_obj_set_pos(status_label, (column+1) * x_spacing + offset_from_left_label, 0);
        lv_label_set_text_fmt(status_label, "%s", LV_SYMBOL_STOP);
        lv_obj_set_style_text_color(status_label, lv_color_background_darkest(), LV_STATE_DEFAULT);
        lvPins[i] = status_label;

        column++;

        if (column >= column_limit) {
            // Add the GPIO number after the last item on a row
            auto* postfix = lv_label_create(row_wrapper);
            lv_label_set_text_fmt(postfix, "%02d", i);
            lv_obj_set_pos(postfix, (column + 1) * x_spacing + offset_from_left_label, 0);

            // Add a new row wrapper underneath the last one
            auto* new_row_wrapper = createGpioRowWrapper(centering_wrapper);
            lv_obj_align_to(new_row_wrapper, row_wrapper, LV_ALIGN_BOTTOM_LEFT, 0, square_spacing);
            row_wrapper = new_row_wrapper;

            column = 0;
        }
    }
    mutex.unlock();

    startTask();
}

void GpioApp::onHide(AppContext& app) {
    stopTask();
}

extern const AppManifest manifest = {
    .id = "Gpio",
    .name = "GPIO",
    .icon = TT_ASSETS_APP_ICON_GPIO,
    .category = Category::System,
    .createApp = create<GpioApp>
};

} // namespace
