#include "Trackball.h"
#include <esp_log.h>

static const char* TAG = "Trackball";

namespace trackball {

static TrackballConfig g_config;
static lv_indev_t* g_indev = nullptr;
static bool g_initialized = false;
static bool g_enabled = true;

// Track last GPIO states for edge detection
static bool g_lastState[5] = {false, false, false, false, false};
// Encoder diff accumulator for navigation
static int16_t g_encDiff = 0;

static void read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    if (!g_initialized || !g_enabled) {
        data->state = LV_INDEV_STATE_RELEASED;
        data->enc_diff = 0;
        return;
    }
    
    const gpio_num_t pins[5] = {
        g_config.pinRight,
        g_config.pinUp,
        g_config.pinLeft,
        g_config.pinDown,
        g_config.pinClick
    };
    
    // Read GPIO states and detect changes (active low with pull-up)
    bool currentStates[5];
    for (int i = 0; i < 5; i++) {
        currentStates[i] = gpio_get_level(pins[i]) == 0;
    }
    
    // Process directional inputs as encoder steps
    // Right/Down = positive diff (next item), Left/Up = negative diff (prev item)
    int16_t diff = 0;
    
    // Right pressed (rising edge)
    if (currentStates[0] && !g_lastState[0]) {
        diff += g_config.movementStep;
    }
    // Up pressed (rising edge)
    if (currentStates[1] && !g_lastState[1]) {
        diff -= g_config.movementStep;
    }
    // Left pressed (rising edge)
    if (currentStates[2] && !g_lastState[2]) {
        diff -= g_config.movementStep;
    }
    // Down pressed (rising edge)
    if (currentStates[3] && !g_lastState[3]) {
        diff += g_config.movementStep;
    }
    
    // Update last states
    for (int i = 0; i < 5; i++) {
        g_lastState[i] = currentStates[i];
    }
    
    // Update encoder diff and button state
    data->enc_diff = diff;
    data->state = currentStates[4] ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    
    // Trigger activity for wake-on-trackball
    if (diff != 0 || currentStates[4]) {
        lv_disp_trig_activity(nullptr);
    }
}

lv_indev_t* init(const TrackballConfig& config) {
    if (g_initialized) {
        ESP_LOGW(TAG, "Trackball already initialized");
        return g_indev;
    }
    
    g_config = config;
    
    // Set default movement step if not specified
    if (g_config.movementStep == 0) {
        g_config.movementStep = 10;
    }
    
    // Configure all GPIO pins as inputs with pull-ups (active low)
    const gpio_num_t pins[5] = {
        config.pinRight,
        config.pinUp,
        config.pinLeft,
        config.pinDown,
        config.pinClick
    };
    
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    
    for (int i = 0; i < 5; i++) {
        io_conf.pin_bit_mask = (1ULL << pins[i]);
        gpio_config(&io_conf);
        g_lastState[i] = gpio_get_level(pins[i]) == 0;
    }
    
    // Register as LVGL encoder input device for group navigation
    g_indev = lv_indev_create();
    lv_indev_set_type(g_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(g_indev, read_cb);
    
    if (g_indev) {
        g_initialized = true;
        ESP_LOGI(TAG, "Trackball initialized as encoder (R:%d U:%d L:%d D:%d Click:%d)",
                 config.pinRight, config.pinUp, config.pinLeft, config.pinDown,
                 config.pinClick);
        return g_indev;
    } else {
        ESP_LOGE(TAG, "Failed to register LVGL input device");
        return nullptr;
    }
}

void deinit() {
    if (g_indev) {
        lv_indev_delete(g_indev);
        g_indev = nullptr;
    }
    g_initialized = false;
    ESP_LOGI(TAG, "Trackball deinitialized");
}

void setMovementStep(uint8_t step) {
    if (step > 0) {
        g_config.movementStep = step;
        ESP_LOGD(TAG, "Movement step set to %d", step);
    }
}

void setEnabled(bool enabled) {
    g_enabled = enabled;
    ESP_LOGI(TAG, "Trackball %s", enabled ? "enabled" : "disabled");
}

}
