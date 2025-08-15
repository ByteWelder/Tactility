#pragma once

#include <Tactility/TactilityCore.h>
#include <Tactility/hal/keyboard/KeyboardDevice.h>

#include <Tca8418.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/pulse_cnt.h>

#include <Tactility/Timer.h>


class TpagerKeyboard : public tt::hal::keyboard::KeyboardDevice {

private:

    lv_indev_t* _Nullable kbHandle = nullptr;
    lv_indev_t* _Nullable encHandle = nullptr;
    pcnt_unit_handle_t encPcntUnit = nullptr;
    gpio_num_t backlightPin = GPIO_NUM_NC;
    ledc_timer_t backlightTimer;
    ledc_channel_t backlightChannel;
    bool backlightOkay = false;
    int backlightImpulseDuty = 0;

    std::shared_ptr<Tca8418> keypad;
    std::unique_ptr<tt::Timer> inputTimer;
    std::unique_ptr<tt::Timer> backlightImpulseTimer;

    void initEncoder(void);
    bool initBacklight(gpio_num_t pin, uint32_t frequencyHz, ledc_timer_t timer, ledc_channel_t channel);
    void processKeyboard();
    void processBacklightImpuse();

public:

    TpagerKeyboard(std::shared_ptr<Tca8418> tca) : keypad(std::move(tca)) {}
    ~TpagerKeyboard() {}

    std::string getName() const final { return "T-Lora Pager Keyboard"; }
    std::string getDescription() const final { return "I2C keyboard with encoder"; }

    bool startLvgl(lv_display_t* display) override;
    bool stopLvgl() override;
    bool isAttached() const override;
    lv_indev_t* _Nullable getLvglIndev() override { return kbHandle; }

    int getEncoderPulses();
    bool setBacklightDuty(uint8_t duty);
    void makeBacklightImpulse();
};

std::shared_ptr<tt::hal::keyboard::KeyboardDevice> createKeyboard();
