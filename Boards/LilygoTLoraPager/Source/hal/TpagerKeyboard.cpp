#include "TpagerKeyboard.h"
#include <Tactility/hal/i2c/I2c.h>
#include <driver/i2c.h>

#include "driver/gpio.h"
#include "freertos/queue.h"

#include <Tactility/Log.h>

#define TAG "tpager_keyboard"

#define ENCODER_A GPIO_NUM_40
#define ENCODER_B GPIO_NUM_41
#define ENCODER_ENTER GPIO_NUM_7
#define BACKLIGHT GPIO_NUM_46

#define KB_ROWS 4
#define KB_COLS 11

// Lowercase Keymap
static constexpr char keymap_lc[KB_ROWS][KB_COLS] = {
    {'\0', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '\n', '\0'},
    {'z', 'x', 'c', 'v', 'b', 'n', 'm', '\0', LV_KEY_BACKSPACE, ' ', '\0'},
    {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}
};

// Uppercase Keymap
static constexpr char keymap_uc[KB_ROWS][KB_COLS] = {
    {'\0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '\n', '\0'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '\0', LV_KEY_BACKSPACE, ' ', '\0'},
    {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}
};

// Symbol Keymap
static constexpr char keymap_sy[KB_ROWS][KB_COLS] = {
    {'\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
    {'.', '/', '+', '-', '=', ':', '\'', '"', '@', '\t', '\0'},
    {'_', '$', ';', '?', '!', ',', '.', '\0', LV_KEY_BACKSPACE, ' ', '\0'},
    {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}
};

static QueueHandle_t keyboardMsg;

static void keyboard_read_callback(lv_indev_t* indev, lv_indev_data_t* data) {
    TpagerKeyboard* kb = (TpagerKeyboard*)lv_indev_get_user_data(indev);
    static bool enter_prev = false;
    char keypress = 0;

    // Defaults
    data->key = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    if (xQueueReceive(keyboardMsg, &keypress, pdMS_TO_TICKS(50)) == pdPASS) {
        data->key = keypress;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

static void encoder_read_callback(lv_indev_t* indev, lv_indev_data_t* data) {
    TpagerKeyboard* kb = (TpagerKeyboard*)lv_indev_get_user_data(indev);
    const int enter_filter_threshold = 2;
    static int enter_filter = 0;
    const int pulses_click = 4;
    static int pulses_prev = 0;
    bool anyinput = false;

    // Defaults
    data->enc_diff = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    int pulses = kb->getEncoderPulses();
    int pulse_diff = (pulses - pulses_prev);
    if ((pulse_diff > pulses_click) || (pulse_diff < -pulses_click)) {
        data->enc_diff = pulse_diff / pulses_click;
        pulses_prev = pulses;
        anyinput = true;
    }

    bool enter = !gpio_get_level(ENCODER_ENTER);
    if (enter && (enter_filter < enter_filter_threshold)) {
        enter_filter++;
    }
    if (!enter && (enter_filter > 0)) {
        enter_filter--;
    }

    if (enter_filter == enter_filter_threshold) {
        data->state = LV_INDEV_STATE_PRESSED;
        anyinput = true;
    }

    if (anyinput) {
        kb->makeBacklightImpulse();
    }
}

void TpagerKeyboard::processKeyboard() {
    static bool shift_pressed = false;
    static bool sym_pressed = false;
    static bool cap_toggle = false;
    static bool cap_toggle_armed = true;
    bool anykey_pressed = false;

    if (keypad->update()) {
        anykey_pressed = (keypad->pressed_key_count > 0);
        for (int i = 0; i < keypad->pressed_key_count; i++) {
            auto row = keypad->pressed_list[i].row;
            auto col = keypad->pressed_list[i].col;
            auto hold = keypad->pressed_list[i].hold_time;

            if ((row == 1) && (col == 10)) {
                sym_pressed = true;
            }
            if ((row == 2) && (col == 7)) {
                shift_pressed = true;
            }
        }

        if ((sym_pressed && shift_pressed) && cap_toggle_armed) {
            cap_toggle = !cap_toggle;
            cap_toggle_armed = false;
        }

        for (int i = 0; i < keypad->pressed_key_count; i++) {
            auto row = keypad->pressed_list[i].row;
            auto col = keypad->pressed_list[i].col;
            auto hold = keypad->pressed_list[i].hold_time;
            char chr = '\0';
            if (sym_pressed) {
                chr = keymap_sy[row][col];
            } else if (shift_pressed || cap_toggle) {
                chr = keymap_uc[row][col];
            } else {
                chr = keymap_lc[row][col];
            }

            if (chr != '\0') xQueueSend(keyboardMsg, (void*)&chr, portMAX_DELAY);
        }

        for (int i = 0; i < keypad->released_key_count; i++) {
            auto row = keypad->released_list[i].row;
            auto col = keypad->released_list[i].col;

            if ((row == 1) && (col == 10)) {
                sym_pressed = false;
            }
            if ((row == 2) && (col == 7)) {
                shift_pressed = false;
            }
        }

        if ((!sym_pressed && !shift_pressed) && !cap_toggle_armed) {
            cap_toggle_armed = true;
        }

        if (anykey_pressed) {
            makeBacklightImpulse();
        }
    }
}

bool TpagerKeyboard::start(lv_display_t* display) {
    backlightOkay = initBacklight(BACKLIGHT, 30000, LEDC_TIMER_0, LEDC_CHANNEL_1);
    initEncoder();
    keypad->init(KB_ROWS, KB_COLS);
    gpio_input_enable(ENCODER_ENTER);

    assert(inputTimer == nullptr);
    inputTimer = std::make_unique<tt::Timer>(tt::Timer::Type::Periodic, [this] {
        processKeyboard();
    });

    assert(backlightImpulseTimer == nullptr);
    backlightImpulseTimer = std::make_unique<tt::Timer>(tt::Timer::Type::Periodic, [this] {
        processBacklightImpuse();
    });

    kbHandle = lv_indev_create();
    lv_indev_set_type(kbHandle, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(kbHandle, &keyboard_read_callback);
    lv_indev_set_display(kbHandle, display);
    lv_indev_set_user_data(kbHandle, this);

    encHandle = lv_indev_create();
    lv_indev_set_type(encHandle, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encHandle, &encoder_read_callback);
    lv_indev_set_display(encHandle, display);
    lv_indev_set_user_data(encHandle, this);

    inputTimer->start(20 / portTICK_PERIOD_MS);
    backlightImpulseTimer->start(50 / portTICK_PERIOD_MS);

    return true;
}

bool TpagerKeyboard::stop() {
    assert(inputTimer);
    inputTimer->stop();
    inputTimer = nullptr;

    assert(backlightImpulseTimer);
    backlightImpulseTimer->stop();
    backlightImpulseTimer = nullptr;

    lv_indev_delete(kbHandle);
    kbHandle = nullptr;
    lv_indev_delete(encHandle);
    encHandle = nullptr;
    return true;
}

bool TpagerKeyboard::isAttached() const {
    return tt::hal::i2c::masterHasDeviceAtAddress(keypad->getPort(), keypad->getAddress(), 100);
}

void TpagerKeyboard::initEncoder(void) {
    const int low_limit = -127;
    const int high_limit = 126;

    // Original implementation based on
    // https://github.com/UsefulElectronics/esp32s3-gc9a01-lvgl/blob/main/main/hardware/rotary_encoder.c
    // Copyright (c) 2023 Ward Almasarani

    // Accum. count makes it that over- and underflows are automatically compensated.
    // Prerequisite: watchpoints at low and high limit
    pcnt_unit_config_t unit_config = {
        .low_limit = low_limit,
        .high_limit = high_limit,
        .flags = {.accum_count = 1},
    };

    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &encPcntUnit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(encPcntUnit, &filter_config));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = ENCODER_A,
        .level_gpio_num = ENCODER_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(encPcntUnit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = ENCODER_B,
        .level_gpio_num = ENCODER_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(encPcntUnit, &chan_b_config, &pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(encPcntUnit, low_limit));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(encPcntUnit, high_limit));

    ESP_ERROR_CHECK(pcnt_unit_enable(encPcntUnit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(encPcntUnit));
    ESP_ERROR_CHECK(pcnt_unit_start(encPcntUnit));
}

int TpagerKeyboard::getEncoderPulses() {
    int pulses = 0;
    pcnt_unit_get_count(encPcntUnit, &pulses);
    return pulses;
}


bool TpagerKeyboard::initBacklight(gpio_num_t pin, uint32_t frequencyHz, ledc_timer_t timer, ledc_channel_t channel) {
    backlightPin = pin;
    backlightTimer = timer;
    backlightChannel = channel;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = backlightTimer,
        .freq_hz = frequencyHz,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        TT_LOG_E(TAG, "Backlight timer config failed");
        return false;
    }

    ledc_channel_config_t ledc_channel = {
        .gpio_num = backlightPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = backlightChannel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = backlightTimer,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0
        }
    };

    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        TT_LOG_E(TAG, "Backlight channel config failed");
    }

    return true;
}

bool TpagerKeyboard::setBacklightDuty(uint8_t duty) {
    if (!backlightOkay) {
        TT_LOG_E(TAG, "Backlight not ready");
        return false;
    }
    return (ledc_set_duty(LEDC_LOW_SPEED_MODE, backlightChannel, duty) == ESP_OK) &&
        (ledc_update_duty(LEDC_LOW_SPEED_MODE, backlightChannel) == ESP_OK);
}

void TpagerKeyboard::makeBacklightImpulse() {
    backlightImpulseDuty = 255;
    setBacklightDuty(backlightImpulseDuty);
}

void TpagerKeyboard::processBacklightImpuse() {
    if (backlightImpulseDuty > 64) {
        backlightImpulseDuty--;
        setBacklightDuty(backlightImpulseDuty);
    }
}

extern std::shared_ptr<Tca8418> tca8418;
std::shared_ptr<tt::hal::keyboard::KeyboardDevice> createKeyboard() {
    keyboardMsg = xQueueCreate(20, sizeof(char));

    return std::make_shared<TpagerKeyboard>(tca8418);
}
