#include "Power.h"

#include <ChargeFromAdcVoltage.h>
#include <EstimatedPower.h>
#include <Tactility/hal/gpio/Gpio.h>
#include <memory>

// ADC enable pin on GPIO37
constexpr auto ADC_CTRL = 37;

std::shared_ptr<tt::hal::power::PowerDevice> createPower() {
    ChargeFromAdcVoltage::Configuration configuration;
    // 2.0 ratio, but +0.11 added as display voltage sag compensation.
    configuration.adcMultiplier = 2.11;

    // Configure the ADC enable pin as an output
    tt::hal::gpio::configure(ADC_CTRL, tt::hal::gpio::Mode::Output, false, false);

    return std::make_shared<EstimatedPower>(configuration);
}