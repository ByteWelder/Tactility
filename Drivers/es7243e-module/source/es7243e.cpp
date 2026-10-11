// SPDX-License-Identifier: Apache-2.0
#include <drivers/es7243e.h>

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/audio_codec.h>
#include <tactility/drivers/audio_codec_adapters.h>
#include <tactility/drivers/i2c_controller.h>
#include <tactility/drivers/i2s_controller.h>
#include <tactility/log.h>

#include <es7243e_adc.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>

constexpr auto* TAG = "ES7243E";

namespace {

constexpr uint32_t NATIVE_SAMPLE_RATE = 16000;
constexpr uint8_t NATIVE_CHANNELS = 2;
constexpr float MAX_INPUT_GAIN_DB = 37.5f;

struct Es7243eData {
    const audio_codec_ctrl_if_t* ctrl_if = nullptr;
    const audio_codec_data_if_t* data_if = nullptr;
    const audio_codec_if_t* codec_if = nullptr;
    esp_codec_dev_handle_t codec_device = nullptr;
    bool is_open = false;
    uint8_t open_bits_per_sample = 16;
    uint8_t open_channels = NATIVE_CHANNELS;
    uint32_t open_sample_rate = 0;
    float input_gain = 1.0f;
};

#define GET_CONFIG(device) (static_cast<const Es7243eConfig*>((device)->config))
#define GET_DATA(device) (static_cast<Es7243eData*>(device_get_driver_data(device)))

// region AudioCodecApi

error_t open(Device* device, const struct AudioCodecStreamConfig* config) {
    auto* data = GET_DATA(device);
    if (data->codec_device == nullptr) {
        return ERROR_RESOURCE;
    }

    if (config->direction != AUDIO_CODEC_DIR_INPUT) {
        LOG_E(TAG, "ES7243E is input-only");
        return ERROR_NOT_SUPPORTED;
    }

    uint8_t channels = (config->channels == 1) ? 1 : NATIVE_CHANNELS;

    if (data->is_open) {
        bool same_config = config->bits_per_sample == data->open_bits_per_sample
            && channels == data->open_channels
            && config->sample_rate == data->open_sample_rate;
        return same_config ? ERROR_NONE : ERROR_RESOURCE;
    }

    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = config->bits_per_sample,
        .channel = channels,
        .channel_mask = 0,
        .sample_rate = config->sample_rate,
        .mclk_multiple = 0,
    };

    if (esp_codec_dev_open(data->codec_device, &sample_info) != ESP_CODEC_DEV_OK) {
        LOG_E(TAG, "Failed to open codec device");
        return ERROR_RESOURCE;
    }

    data->open_bits_per_sample = config->bits_per_sample;
    data->open_channels = channels;
    data->open_sample_rate = config->sample_rate;
    data->is_open = true;
    return ERROR_NONE;
}

error_t close(Device* device) {
    auto* data = GET_DATA(device);
    if (data->codec_device == nullptr) {
        return ERROR_RESOURCE;
    }

    if (data->is_open) {
        esp_codec_dev_close(data->codec_device);
        data->is_open = false;
    }

    return ERROR_NONE;
}

error_t read(Device* device, void* buffer, size_t size, size_t* bytes_read, TickType_t timeout) {
    (void) timeout;
    auto* data = GET_DATA(device);
    if (!data->is_open) {
        return ERROR_RESOURCE;
    }

    int result = esp_codec_dev_read(data->codec_device, buffer, (int) size);
    if (result < 0) {
        return ERROR_RESOURCE;
    }
    *bytes_read = (size_t) result;
    return ERROR_NONE;
}

error_t write(Device* device, const void* buffer, size_t size, size_t* bytes_written, TickType_t timeout) {
    (void) device;
    (void) buffer;
    (void) size;
    (void) bytes_written;
    (void) timeout;
    return ERROR_NOT_SUPPORTED;
}

error_t set_volume(Device* device, AudioCodecDirection direction, float volume_percent) {
    auto* data = GET_DATA(device);
    if (data->codec_device == nullptr || direction != AUDIO_CODEC_DIR_INPUT) {
        return ERROR_NOT_SUPPORTED;
    }

    float db = (volume_percent / 100.0f) * MAX_INPUT_GAIN_DB;
    return (esp_codec_dev_set_in_gain(data->codec_device, db) == ESP_CODEC_DEV_OK) ? ERROR_NONE : ERROR_RESOURCE;
}

error_t get_volume(Device* device, AudioCodecDirection direction, float* volume_percent) {
    auto* data = GET_DATA(device);
    if (data->codec_device == nullptr || direction != AUDIO_CODEC_DIR_INPUT) {
        return ERROR_NOT_SUPPORTED;
    }

    float db = 0.0f;
    if (esp_codec_dev_get_in_gain(data->codec_device, &db) != ESP_CODEC_DEV_OK) {
        return ERROR_RESOURCE;
    }
    *volume_percent = (db / MAX_INPUT_GAIN_DB) * 100.0f;
    return ERROR_NONE;
}

error_t set_mute(Device* device, AudioCodecDirection direction, bool muted) {
    auto* data = GET_DATA(device);
    if (data->codec_device == nullptr || direction != AUDIO_CODEC_DIR_INPUT) {
        return ERROR_NOT_SUPPORTED;
    }

    return (esp_codec_dev_set_in_mute(data->codec_device, muted) == ESP_CODEC_DEV_OK) ? ERROR_NONE : ERROR_RESOURCE;
}

error_t get_mute(Device* device, AudioCodecDirection direction, bool* muted) {
    auto* data = GET_DATA(device);
    if (data->codec_device == nullptr || direction != AUDIO_CODEC_DIR_INPUT) {
        return ERROR_NOT_SUPPORTED;
    }

    return (esp_codec_dev_get_in_mute(data->codec_device, muted) == ESP_CODEC_DEV_OK) ? ERROR_NONE : ERROR_RESOURCE;
}

error_t get_native_channels(Device* device, AudioCodecDirection direction, uint8_t* channels) {
    (void) device;
    if (direction != AUDIO_CODEC_DIR_INPUT) {
        return ERROR_NOT_SUPPORTED;
    }
    *channels = NATIVE_CHANNELS;
    return ERROR_NONE;
}

error_t get_native_sample_rate(Device* device, AudioCodecDirection direction, uint32_t* rate_hz) {
    (void) device;
    if (direction != AUDIO_CODEC_DIR_INPUT) {
        return ERROR_NOT_SUPPORTED;
    }
    *rate_hz = NATIVE_SAMPLE_RATE;
    return ERROR_NONE;
}

error_t get_capabilities(Device* device, AudioCodecDirection* supported_directions) {
    (void) device;
    *supported_directions = AUDIO_CODEC_DIR_INPUT;
    return ERROR_NONE;
}

error_t get_input_gain_multiplier(Device* device, float* gain) {
    *gain = GET_DATA(device)->input_gain;
    return ERROR_NONE;
}

static const struct AudioCodecApi API = {
    .open = open,
    .close = close,
    .read = read,
    .write = write,
    .set_volume = set_volume,
    .get_volume = get_volume,
    .set_mute = set_mute,
    .get_mute = get_mute,
    .get_native_sample_rate = get_native_sample_rate,
    .get_native_channels = get_native_channels,
    .get_capabilities = get_capabilities,
    .get_input_gain_multiplier = get_input_gain_multiplier,
};

// endregion

// region Driver lifecycle

void delete_interfaces(Es7243eData* data) {
    // The delete functions close their interface first
    if (data->codec_device != nullptr) {
        esp_codec_dev_delete(data->codec_device);
    }
    if (data->codec_if != nullptr) {
        audio_codec_delete_codec_if(data->codec_if);
    }
    if (data->data_if != nullptr) {
        audio_codec_delete_data_if(data->data_if);
    }
    if (data->ctrl_if != nullptr) {
        audio_codec_delete_ctrl_if(data->ctrl_if);
    }
}

error_t start_device(Device* device) {
    const auto* config = GET_CONFIG(device);

    auto* i2c_controller = device_get_parent(device);
    if (i2c_controller == nullptr || device_get_type(i2c_controller) != &I2C_CONTROLLER_TYPE) {
        LOG_E(TAG, "Parent is not an I2C controller");
        return ERROR_RESOURCE;
    }

    auto* i2s_controller = config->i2s_device;
    if (i2s_controller == nullptr || device_get_type(i2s_controller) != &I2S_CONTROLLER_TYPE) {
        LOG_E(TAG, "I2S controller device is not valid");
        return ERROR_RESOURCE;
    }

    auto* data = new Es7243eData();
    data->input_gain = (float) config->input_gain_percent / 100.0f;

    data->ctrl_if = audio_codec_adapter_new_i2c_ctrl(i2c_controller, config->address);
    data->data_if = audio_codec_adapter_new_i2s_data(i2s_controller);
    bool ok = data->ctrl_if != nullptr && data->data_if != nullptr;
    if (!ok) {
        LOG_E(TAG, "Failed to create adapters");
    }

    if (ok && data->ctrl_if->open(data->ctrl_if, nullptr, 0) != ESP_CODEC_DEV_OK) {
        LOG_E(TAG, "Failed to open control interface");
        ok = false;
    }

    if (ok && data->data_if->open(data->data_if, nullptr, 0) != ESP_CODEC_DEV_OK) {
        LOG_E(TAG, "Failed to open data interface");
        ok = false;
    }

    if (ok) {
        es7243e_codec_cfg_t codec_config = {};
        codec_config.ctrl_if = data->ctrl_if;
        data->codec_if = es7243e_codec_new(&codec_config);
        if (data->codec_if == nullptr) {
            LOG_E(TAG, "Failed to create ES7243E codec interface");
            ok = false;
        }
    }

    if (ok) {
        esp_codec_dev_cfg_t dev_config = {
            .dev_type = ESP_CODEC_DEV_TYPE_IN,
            .codec_if = data->codec_if,
            .data_if = data->data_if,
        };
        data->codec_device = esp_codec_dev_new(&dev_config);
        if (data->codec_device == nullptr) {
            LOG_E(TAG, "Failed to create codec device");
            ok = false;
        }
    }

    if (!ok) {
        delete_interfaces(data);
        delete data;
        return ERROR_RESOURCE;
    }

    device_set_driver_data(device, data);
    return ERROR_NONE;
}

error_t stop_device(Device* device) {
    auto* data = GET_DATA(device);
    if (data == nullptr) {
        return ERROR_NONE;
    }

    if (data->is_open) {
        esp_codec_dev_close(data->codec_device);
    }
    delete_interfaces(data);

    device_set_driver_data(device, nullptr);
    delete data;
    return ERROR_NONE;
}

// endregion

} // namespace

extern "C" {

Driver es7243e_driver = {
    .name = "es7243e",
    .compatible = (const char*[]) { "everest,es7243e", nullptr },
    .start_device = start_device,
    .stop_device = stop_device,
    .probe = nullptr,
    .api = &API,
    .device_type = &AUDIO_CODEC_TYPE,
    .internal = nullptr,
};

}
