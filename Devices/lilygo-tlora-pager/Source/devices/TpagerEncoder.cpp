#include "TpagerEncoder.h"

#include <Tactility/Logger.h>
#include <driver/gpio.h>

static const auto LOGGER = tt::Logger("TpagerEncoder");

constexpr auto ENCODER_A = GPIO_NUM_40;
constexpr auto ENCODER_B = GPIO_NUM_41;
constexpr auto ENCODER_ENTER = GPIO_NUM_7;

void TpagerEncoder::readCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    TpagerEncoder* encoder = static_cast<TpagerEncoder*>(lv_indev_get_user_data(indev));
    constexpr int enter_filter_threshold = 2;
    static int enter_filter = 0;
    constexpr int pulses_click = 4;
    static int pulses_prev = 0;

    // Defaults
    data->enc_diff = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    int pulses = encoder->getEncoderPulses();
    int pulse_diff = (pulses - pulses_prev);
    if ((pulse_diff > pulses_click) || (pulse_diff < -pulses_click)) {
        data->enc_diff = pulse_diff / pulses_click;
        pulses_prev = pulses;
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
    }
}

bool TpagerEncoder::initEncoder() {
    assert(encPcntUnit == nullptr);

    constexpr int LOW_LIMIT = -127;
    constexpr int HIGH_LIMIT = 126;

    // Accum. count makes it that over- and underflows are automatically compensated.
    // Prerequisite: watchpoints at low and high limit
    pcnt_unit_config_t unit_config = {
        .low_limit = LOW_LIMIT,
        .high_limit = HIGH_LIMIT,
        .intr_priority = 0,
        .flags = {
            .accum_count = 1
        },
    };

    if (pcnt_new_unit(&unit_config, &encPcntUnit) != ESP_OK) {
        LOGGER.error("Pulsecounter initialization failed");
        return false;
    }

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };

    if (pcnt_unit_set_glitch_filter(encPcntUnit, &filter_config) != ESP_OK) {
        LOGGER.error("Pulsecounter glitch filter config failed");
        return false;
    }

    pcnt_chan_config_t chan_1_config = {
        .edge_gpio_num = ENCODER_B,
        .level_gpio_num = ENCODER_A,
        .flags {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0
        }
    };

    pcnt_chan_config_t chan_2_config = {
        .edge_gpio_num = ENCODER_A,
        .level_gpio_num = ENCODER_B,
        .flags {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0
        }
    };

    pcnt_channel_handle_t pcnt_chan_1 = nullptr;
    pcnt_channel_handle_t pcnt_chan_2 = nullptr;

    if ((pcnt_new_channel(encPcntUnit, &chan_1_config, &pcnt_chan_1) != ESP_OK) ||
        (pcnt_new_channel(encPcntUnit, &chan_2_config, &pcnt_chan_2) != ESP_OK)) {
        LOGGER.error("Pulsecounter channel config failed");
        return false;
    }

    // Second argument is rising edge, third argument is falling edge
    if ((pcnt_channel_set_edge_action(pcnt_chan_1, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE) != ESP_OK) ||
        (pcnt_channel_set_edge_action(pcnt_chan_2, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE) != ESP_OK)) {
        LOGGER.error("Pulsecounter edge action config failed");
        return false;
    }

    // Second argument is low level, third argument is high level
    if ((pcnt_channel_set_level_action(pcnt_chan_1, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE) != ESP_OK) ||
        (pcnt_channel_set_level_action(pcnt_chan_2, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE) != ESP_OK)) {
        LOGGER.error("Pulsecounter level action config failed");
        return false;
    }

    if ((pcnt_unit_add_watch_point(encPcntUnit, LOW_LIMIT) != ESP_OK) ||
        (pcnt_unit_add_watch_point(encPcntUnit, HIGH_LIMIT) != ESP_OK)) {
        LOGGER.error("Pulsecounter watch point config failed");
        return false;
    }

    if (pcnt_unit_enable(encPcntUnit) != ESP_OK) {
        LOGGER.error("Pulsecounter could not be enabled");
        return false;
    }

    if (pcnt_unit_clear_count(encPcntUnit) != ESP_OK) {
        LOGGER.error("Pulsecounter could not be cleared");
        return false;
    }

    if (pcnt_unit_start(encPcntUnit) != ESP_OK) {
        LOGGER.error("Pulsecounter could not be started");
        return false;
    }

    return true;
}

int TpagerEncoder::getEncoderPulses() const {
    int pulses = 0;
    pcnt_unit_get_count(encPcntUnit, &pulses);
    return pulses;
}

bool TpagerEncoder::deinitEncoder() {
    assert(encPcntUnit != nullptr);

    if (pcnt_unit_stop(encPcntUnit) != ESP_OK) {
        LOGGER.warn("Failed to stop encoder");
    }

    if (pcnt_del_unit(encPcntUnit) != ESP_OK) {
        LOGGER.warn("Failed to delete encoder");
        return false;
    }

    LOGGER.info("Deinitialized");

    return true;
}


bool TpagerEncoder::startLvgl(lv_display_t* display) {
    if (encPcntUnit == nullptr && !initEncoder()) {
        return false;
    }

    gpio_input_enable(ENCODER_ENTER);

    encHandle = lv_indev_create();

    lv_indev_set_type(encHandle, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encHandle, &readCallback);
    lv_indev_set_display(encHandle, display);
    lv_indev_set_user_data(encHandle, this);

    return true;
}

bool TpagerEncoder::stopLvgl() {
    lv_indev_delete(encHandle);
    encHandle = nullptr;

    if (encPcntUnit != nullptr && !deinitEncoder()) {
        // We're not returning false as LVGL as effectively deinitialized
        LOGGER.warn("Deinitialization failed");
    }

    return true;
}
