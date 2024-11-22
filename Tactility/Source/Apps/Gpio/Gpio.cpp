#include "Mutex.h"
#include "Thread.h"
#include "Services/Loader/Loader.h"
#include "Ui/Toolbar.h"

#include "GpioHal.h"
#include "Ui/LvglSync.h"

namespace tt::app::gpio {

typedef struct {
    lv_obj_t* lv_pins[GPIO_NUM_MAX];
    uint8_t pin_states[GPIO_NUM_MAX];
    Thread* thread;
    Mutex* mutex;
    bool thread_interrupted;
} Gpio;

static void lock(Gpio* gpio) {
    tt_check(tt_mutex_acquire(gpio->mutex, 1000) == TtStatusOk);
}

static void unlock(Gpio* gpio) {
    tt_check(tt_mutex_release(gpio->mutex) == TtStatusOk);
}

static void update_pin_states(Gpio* gpio) {
    lock(gpio);
    // Update pin states
    for (int i = 0; i < GPIO_NUM_MAX; ++i) {
#ifdef ESP_PLATFORM
        gpio->pin_states[i] = gpio_get_level((gpio_num_t)i);
#else
        gpio->pin_states[i] = gpio_get_level(i);
#endif
    }
    unlock(gpio);
}

static void update_pin_widgets(Gpio* gpio) {
    if (lvgl::lock(100)) {
        lock(gpio);
        for (int j = 0; j < GPIO_NUM_MAX; ++j) {
            int level = gpio->pin_states[j];
            lv_obj_t* label = gpio->lv_pins[j];
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
        unlock(gpio);
    }
}

static lv_obj_t* create_gpio_row_wrapper(lv_obj_t* parent) {
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    return wrapper;
}

// region Task

static int32_t gpio_task(void* context) {
    Gpio* gpio = (Gpio*)context;
    bool interrupted = false;

    while (!interrupted) {
        delay_ms(100);

        update_pin_states(gpio);
        update_pin_widgets(gpio);

        lock(gpio);
        interrupted = gpio->thread_interrupted;
        unlock(gpio);
    }

    return 0;
}

static void task_start(Gpio* gpio) {
    tt_assert(gpio->thread == nullptr);
    lock(gpio);
    gpio->thread = new Thread(
        "gpio",
        4096,
        &gpio_task,
        gpio
    );
    gpio->thread_interrupted = false;
    gpio->thread->start();
    unlock(gpio);
}

static void task_stop(Gpio* gpio) {
    tt_assert(gpio->thread);
    lock(gpio);
    gpio->thread_interrupted = true;
    unlock(gpio);

    gpio->thread->join();

    lock(gpio);
    delete gpio->thread;
    gpio->thread = nullptr;
    unlock(gpio);
}

// endregion Task

// region App lifecycle

static void app_show(App app, lv_obj_t* parent) {
    auto* gpio = static_cast<Gpio*>(tt_app_get_data(app));

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* toolbar = lvgl::toolbar_create_for_app(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    // Main content wrapper, enables scrolling content without scrolling the toolbar
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    uint8_t column = 0;
    uint8_t column_limit = 10;
    int32_t x_spacing = 20;

    lv_obj_t* row_wrapper = create_gpio_row_wrapper(wrapper);
    lv_obj_align(row_wrapper, LV_ALIGN_TOP_MID, 0, 0);

    lock(gpio);
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
        gpio->lv_pins[i] = status_label;

        column++;

        if (column >= column_limit) {
            // Add the GPIO number after the last item on a row
            lv_obj_t* postfix = lv_label_create(row_wrapper);
            lv_label_set_text_fmt(postfix, "%02d", i);
            lv_obj_set_pos(postfix, (int32_t)((column+1) * x_spacing), 0);

            // Add a new row wrapper underneath the last one
            lv_obj_t* new_row_wrapper = create_gpio_row_wrapper(wrapper);
            lv_obj_align_to(new_row_wrapper, row_wrapper, LV_ALIGN_BOTTOM_LEFT, 0, 4);
            row_wrapper = new_row_wrapper;

            column = 0;
        }
    }
    unlock(gpio);

    task_start(gpio);
}

static void on_hide(App app) {
    auto* gpio = static_cast<Gpio*>(tt_app_get_data(app));
    task_stop(gpio);
}

static void on_start(App app) {
    auto* gpio = static_cast<Gpio*>(malloc(sizeof(Gpio)));
    *gpio = (Gpio) {
        .lv_pins = { nullptr },
        .pin_states = { 0 },
        .thread = nullptr,
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .thread_interrupted = true,
    };
    tt_app_set_data(app, gpio);
}

static void on_stop(App app) {
    auto* gpio = static_cast<Gpio*>(tt_app_get_data(app));
    tt_mutex_free(gpio->mutex);
    free(gpio);
}

// endregion App lifecycle

extern const AppManifest manifest = {
    .id = "gpio",
    .name = "GPIO",
    .type = AppTypeSystem,
    .on_start = &on_start,
    .on_stop = &on_stop,
    .on_show = &app_show,
    .on_hide = &on_hide
};

} // namespace
