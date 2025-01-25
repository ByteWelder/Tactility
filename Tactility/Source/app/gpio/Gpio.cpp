#include "Mutex.h"
#include "Thread.h"
#include "service/loader/Loader.h"
#include "lvgl/Toolbar.h"

#include "GpioHal.h"
#include "lvgl/LvglSync.h"
#include "Timer.h"

namespace tt::app::gpio {

extern const AppManifest manifest;

class GpioApp : public App {

private:

    lv_obj_t* lvPins[GPIO_NUM_MAX] = {0 };
    uint8_t pinStates[GPIO_NUM_MAX] = {0 };
    std::unique_ptr<Timer> timer;
    Mutex mutex;

    static lv_obj_t* createGpioRowWrapper(lv_obj_t* parent);
    static void onTimer(TT_UNUSED std::shared_ptr<void> context);

public:

    void lock() const {
        tt_check(mutex.acquire(1000) == TtStatusOk);
    }

    void unlock() const {
        tt_check(mutex.release() == TtStatusOk);
    }

    void onShow(AppContext& app, lv_obj_t* parent) override;
    void onHide(AppContext& app) override;

    void startTask();
    void stopTask();

    void updatePinStates();
    void updatePinWidgets();
};

void GpioApp::updatePinStates() {
    lock();
    // Update pin states
    for (int i = 0; i < GPIO_NUM_MAX; ++i) {
#ifdef ESP_PLATFORM
        pinStates[i] = gpio_get_level((gpio_num_t)i);
#else
        pinStates[i] = gpio_get_level(i);
#endif
    }
    unlock();
}

void GpioApp::updatePinWidgets() {
    if (lvgl::lock(100)) {
        lock();
        for (int j = 0; j < GPIO_NUM_MAX; ++j) {
            int level = pinStates[j];
            lv_obj_t* label = lvPins[j];
            void* label_user_data = lv_obj_get_user_data(label);
            // The user data stores the state, so we can avoid unnecessary updates
            if ((void*)level != label_user_data) {
                lv_obj_set_user_data(label, (void*)level);
                if (level == 0) {
                    lv_obj_set_style_text_color(label, lv_color_black(), 0);
                } else {
                    lv_obj_set_style_text_color(label, lv_color_make(0, 200, 0), 0);
                }
            }
        }
        lvgl::unlock();
        unlock();
    }
}

lv_obj_t* GpioApp::createGpioRowWrapper(lv_obj_t* parent) {
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    return wrapper;
}

// region Task

void GpioApp::onTimer(TT_UNUSED std::shared_ptr<void> context) {
    auto appContext = service::loader::getCurrentAppContext();
    if (appContext->getManifest().id == manifest.id) {
        auto app = std::static_pointer_cast<GpioApp>(appContext->getApp());
        if (app != nullptr) {
            app->updatePinStates();
            app->updatePinWidgets();
        }
    }
}

void GpioApp::startTask() {
    lock();
    assert(timer == nullptr);
    timer = std::make_unique<Timer>(
        Timer::Type::Periodic,
        &onTimer
    );
    timer->start(100 / portTICK_PERIOD_MS);
    unlock();
}

void GpioApp::stopTask() {
    assert(timer);

    timer->stop();
    timer = nullptr;
}

// endregion Task


void GpioApp::onShow(AppContext& app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    // Main content wrapper, enables scrolling content without scrolling the toolbar
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    auto* display = lv_obj_get_display(parent);
    auto horizontal_px = lv_display_get_horizontal_resolution(display);
    auto vertical_px = lv_display_get_vertical_resolution(display);
    bool is_landscape_display = horizontal_px > vertical_px;

    int32_t x_spacing = 20;
    uint8_t column = 0;
    uint8_t column_limit = is_landscape_display ? 10 : 5;

    lv_obj_t* row_wrapper = createGpioRowWrapper(wrapper);
    lv_obj_align(row_wrapper, LV_ALIGN_TOP_MID, 0, 0);

    lock();
    for (int i = GPIO_NUM_MIN; i < GPIO_NUM_MAX; ++i) {

        // Add the GPIO number before the first item on a row
        if (column == 0) {
            lv_obj_t* prefix = lv_label_create(row_wrapper);
            lv_label_set_text_fmt(prefix, "%02d", i);
        }

        // Add a new GPIO status indicator
        lv_obj_t* status_label = lv_label_create(row_wrapper);
        lv_obj_set_pos(status_label, (int32_t)((column+1) * x_spacing), 0);
        lv_label_set_text_fmt(status_label, "%s", LV_SYMBOL_STOP);
        lvPins[i] = status_label;

        column++;

        if (column >= column_limit) {
            // Add the GPIO number after the last item on a row
            lv_obj_t* postfix = lv_label_create(row_wrapper);
            lv_label_set_text_fmt(postfix, "%02d", i);
            lv_obj_set_pos(postfix, (int32_t)((column+1) * x_spacing), 0);

            // Add a new row wrapper underneath the last one
            lv_obj_t* new_row_wrapper = createGpioRowWrapper(wrapper);
            lv_obj_align_to(new_row_wrapper, row_wrapper, LV_ALIGN_BOTTOM_LEFT, 0, 4);
            row_wrapper = new_row_wrapper;

            column = 0;
        }
    }
    unlock();

    startTask();
}

void GpioApp::onHide(AppContext& app) {
    stopTask();
}

extern const AppManifest manifest = {
    .id = "Gpio",
    .name = "GPIO",
    .type = Type::System,
    .createApp = create<GpioApp>
};

} // namespace
