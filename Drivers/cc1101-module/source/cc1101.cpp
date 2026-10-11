// SPDX-License-Identifier: Apache-2.0
#include <new>

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/esp32_spi.h>
#include <tactility/drivers/gpio.h>
#include <tactility/drivers/gpio_controller.h>
#include <tactility/drivers/radio.h>
#include <tactility/drivers/spi_controller.h>
#include <tactility/log.h>
#include <tactility/module.h>

#include <drivers/cc1101.h>
#include <drivers/cc1101_radio.h>

#include <driver/gpio.h>

#define TAG "cc1101"

#define GET_CONFIG(device) ((const struct Cc1101Config*)device->config)
#define GET_DATA(device) ((struct Cc1101Internal*)device_get_driver_data(device))

extern "C" {

struct Cc1101Internal {
    Cc1101Radio* radio = nullptr;
    GpioDescriptor* pin_gdo0 = nullptr;
    GpioDescriptor* pin_band_switch_0 = nullptr;
    GpioDescriptor* pin_band_switch_1 = nullptr;

    void release_pins() {
        release_pin(&pin_gdo0);
        release_pin(&pin_band_switch_0);
        release_pin(&pin_band_switch_1);
    }

private:
    static void release_pin(GpioDescriptor** descriptor) {
        if (*descriptor != nullptr) {
            gpio_descriptor_release(*descriptor);
            *descriptor = nullptr;
        }
    }
};

static error_t acquire_optional_output(const struct GpioPinSpec& spec, const char* pin_name, GpioDescriptor** out_descriptor) {
    if (spec.gpio_controller == nullptr) {
        *out_descriptor = nullptr;
        return ERROR_NONE;
    }

    auto* descriptor = gpio_descriptor_acquire(spec.gpio_controller, spec.pin, GPIO_FLAG_DIRECTION_OUTPUT, GPIO_OWNER_GPIO);
    if (descriptor == nullptr) {
        LOG_E(TAG, "Failed to acquire pin \"%s\"", pin_name);
        return ERROR_RESOURCE;
    }

    *out_descriptor = descriptor;
    return ERROR_NONE;
}

static error_t start(Device* device) {
    LOG_I(TAG, "start %s", device->name);

    auto* parent = device_get_parent(device);
    if (device_get_type(parent) != &SPI_CONTROLLER_TYPE) {
        LOG_E(TAG, "Parent device is not an SPI controller");
        return ERROR_INVALID_STATE;
    }

    auto* config = GET_CONFIG(device);

    // The chip-select lookup uses the node's unit address, so reg must agree with it
    if (config->reg != device->address) {
        LOG_E(TAG, "reg (%d) does not match the node's unit address (%d)", (int)config->reg, (int)device->address);
        return ERROR_INVALID_STATE;
    }

    if (config->pin_gdo0.gpio_controller == nullptr) {
        LOG_E(TAG, "Pin \"pin-gdo0\" is not set");
        return ERROR_INVALID_STATE;
    }

    auto* data = new (std::nothrow) Cc1101Internal();
    if (data == nullptr) return ERROR_OUT_OF_MEMORY;

    // GDO0 keeps a plain input here. The radio thread adds the interrupt flags when it attaches the handler.
    data->pin_gdo0 = gpio_descriptor_acquire(config->pin_gdo0.gpio_controller, config->pin_gdo0.pin, GPIO_FLAG_DIRECTION_INPUT, GPIO_OWNER_GPIO);
    if (data->pin_gdo0 == nullptr) {
        LOG_E(TAG, "Failed to acquire pin \"pin-gdo0\"");
        delete data;
        return ERROR_RESOURCE;
    }

    gpio_num_t pin_gdo0 = GPIO_NUM_NC;
    if (gpio_descriptor_get_native_pin_number(data->pin_gdo0, &pin_gdo0) != ERROR_NONE) {
        LOG_E(TAG, "Pin \"pin-gdo0\" must be an SoC GPIO");
        data->release_pins();
        delete data;
        return ERROR_RESOURCE;
    }

    if (acquire_optional_output(config->pin_band_switch_0, "pin-band-switch-0", &data->pin_band_switch_0) != ERROR_NONE ||
        acquire_optional_output(config->pin_band_switch_1, "pin-band-switch-1", &data->pin_band_switch_1) != ERROR_NONE) {
        data->release_pins();
        delete data;
        return ERROR_RESOURCE;
    }

    struct GpioPinSpec cs_pin_spec;
    if (esp32_spi_get_cs_pin(device, &cs_pin_spec) != ERROR_NONE) {
        LOG_E(TAG, "Failed to get CS pin from parent SPI controller");
        data->release_pins();
        delete data;
        return ERROR_RESOURCE;
    }

    auto* spi_config = static_cast<const Esp32SpiConfig*>(parent->config);

    const Cc1101Radio::Settings settings = {
        .device = device,
        .spi_controller = parent,
        .spi_host = spi_config->host,
        .spi_frequency_hz = (int)config->spi_frequency_khz * 1000,
        .pin_cs = static_cast<gpio_num_t>(cs_pin_spec.pin),
        .pin_gdo0 = pin_gdo0,
        .gdo0 = data->pin_gdo0,
        .band_switch_0 = data->pin_band_switch_0,
        .band_switch_1 = data->pin_band_switch_1,
        .band_switch_low = static_cast<uint8_t>(config->band_switch_low),
        .band_switch_mid = static_cast<uint8_t>(config->band_switch_mid),
        .band_switch_high = static_cast<uint8_t>(config->band_switch_high),
    };

    LOG_I(
        TAG,
        "%s: CS=%d GDO0=%d, SPI %u kHz, band switch %s",
        device->name,
        (int)settings.pin_cs,
        (int)pin_gdo0,
        (unsigned)config->spi_frequency_khz,
        (data->pin_band_switch_0 != nullptr || data->pin_band_switch_1 != nullptr) ? "yes" : "no"
    );

    data->radio = new (std::nothrow) Cc1101Radio(settings);
    if (data->radio == nullptr) {
        data->release_pins();
        delete data;
        return ERROR_OUT_OF_MEMORY;
    }

    // A miswired or unpowered module fails at device start instead of later on the radio thread
    if (data->radio->probe() != ERROR_NONE) {
        delete data->radio;
        data->release_pins();
        delete data;
        return ERROR_RESOURCE;
    }

    device_set_driver_data(device, data);
    return ERROR_NONE;
}

static error_t stop(Device* device) {
    LOG_I(TAG, "stop %s", device->name);

    auto* data = GET_DATA(device);
    if (data == nullptr) return ERROR_NONE;

    // Stops the radio thread before teardown
    delete data->radio;
    data->radio = nullptr;

    data->release_pins();
    device_set_driver_data(device, nullptr);
    delete data;
    return ERROR_NONE;
}

// region RadioApi

static Cc1101Radio* get_radio(Device* device) {
    auto* data = GET_DATA(device);
    return (data != nullptr) ? data->radio : nullptr;
}

static error_t api_get_radio_state(Device* device, enum RadioState* state) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    *state = radio->getState();
    return ERROR_NONE;
}

static error_t api_set_enabled(Device* device, bool enabled) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    return radio->setEnabled(enabled);
}

static error_t api_set_modulation(Device* device, enum RadioModulation modulation) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    return radio->setModulation(modulation);
}

static error_t api_get_modulation(Device* device, enum RadioModulation* modulation) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    *modulation = radio->getModulation();
    return ERROR_NONE;
}

static bool api_can_transmit(Device* device, enum RadioModulation modulation) {
    auto* radio = get_radio(device);
    return (radio != nullptr) && radio->canTransmit(modulation);
}

static bool api_can_receive(Device* device, enum RadioModulation modulation) {
    auto* radio = get_radio(device);
    return (radio != nullptr) && radio->canReceive(modulation);
}

static error_t api_set_parameter(Device* device, enum RadioParameter parameter, int32_t value) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    return radio->setParameter(parameter, value);
}

static error_t api_get_parameter(Device* device, enum RadioParameter parameter, int32_t* value) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    return radio->getParameter(parameter, value);
}

static error_t api_transmit(Device* device, const uint8_t* data, size_t length, RadioTxId* id) {
    auto* radio = get_radio(device);
    if (radio == nullptr) return ERROR_INVALID_STATE;
    return radio->transmit(data, length, id);
}

static const struct RadioApi cc1101_radio_api = {
    .get_radio_state = api_get_radio_state,
    .set_enabled = api_set_enabled,
    .set_modulation = api_set_modulation,
    .get_modulation = api_get_modulation,
    .can_transmit = api_can_transmit,
    .can_receive = api_can_receive,
    .set_parameter = api_set_parameter,
    .get_parameter = api_get_parameter,
    .transmit = api_transmit,
};

// endregion

Driver cc1101_driver = {
    .name = "cc1101",
    .compatible = (const char*[]) { "ti,cc1101", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = &cc1101_radio_api,
    .device_type = &RADIO_TYPE,
    .internal = nullptr
};

} // extern "C"
