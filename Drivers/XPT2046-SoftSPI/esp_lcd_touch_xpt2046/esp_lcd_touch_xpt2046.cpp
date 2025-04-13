#include "esp_lcd_touch_xpt2046.h"
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_log.h>
#include <esp_rom_gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <cstring>
#include "esp_timer.h"


static const char* TAG = "xpt2046_softspi";

#define ESP_GOTO_ON_FALSE_LOG(a, err_code, tag, msg, ...) do { \
    if (!(a)) { \
        ESP_LOGE(tag, msg, ##__VA_ARGS__); \
        err_code = ESP_FAIL; \
        goto err; \
    } \
} while (0)

#define ESP_GOTO_ON_ERROR_LOG(a, err_code, tag, msg, ...) do { \
    esp_err_t ret = (a); \
    if (ret != ESP_OK) { \
        ESP_LOGE(tag, msg, ##__VA_ARGS__); \
        err_code = ret; \
        goto err; \
    } \
} while (0)

#define XPT2046_PD_BITS (0x01)
enum xpt2046_registers {
    Z_VALUE_1 = 0xB0 | XPT2046_PD_BITS,
    Z_VALUE_2 = 0xC0 | XPT2046_PD_BITS,
    Y_POSITION = 0x90 | XPT2046_PD_BITS,
    X_POSITION = 0xD0 | XPT2046_PD_BITS,
};

static const uint16_t XPT2046_ADC_LIMIT = 4096;
static const uint16_t Z_THRESHOLD = 50; // Lowered for sensitivity

typedef struct {
    esp_lcd_touch_t base;
    void* user_data;
} esp_lcd_touch_xpt2046_t;

XPT2046_SoftSPI::XPT2046_SoftSPI(const Config& config)
    : handle_(nullptr), indev_(nullptr),
      spi_(std::make_unique<SoftSPI>(config.miso_pin, config.mosi_pin, config.sck_pin)),
      cs_pin_(config.cs_pin) {
    esp_err_t err = ESP_OK;
    esp_lcd_touch_xpt2046_t* tp = (esp_lcd_touch_xpt2046_t*)calloc(1, sizeof(esp_lcd_touch_xpt2046_t));
    ESP_GOTO_ON_FALSE_LOG(tp, err, TAG, "No memory for XPT2046 state");
    handle_ = (esp_lcd_touch_handle_t)tp;

    tp->base.read_data = read_data;
    tp->base.get_xy = get_xy;
    tp->base.del = del;
    tp->base.data.lock.owner = portMUX_FREE_VAL;
    tp->user_data = this;
    memcpy(&tp->base.config, &config.touch_config.base, sizeof(esp_lcd_touch_config_t));

    gpio_set_direction(cs_pin_, GPIO_MODE_OUTPUT);
    gpio_set_level(cs_pin_, 1);

    if (config.touch_config.base.int_gpio_num != GPIO_NUM_NC) {
        ESP_GOTO_ON_FALSE_LOG(GPIO_IS_VALID_GPIO(config.touch_config.base.int_gpio_num), err, TAG, "Invalid IRQ pin");
        gpio_config_t cfg = {
            .pin_bit_mask = BIT64(config.touch_config.base.int_gpio_num),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE
        };
        ESP_GOTO_ON_ERROR_LOG(gpio_config(&cfg), err, TAG, "IRQ config failed");
    }

    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_, lvgl_read_cb);
    lv_indev_set_user_data(indev_, this);

    ESP_LOGI(TAG, "XPT2046 SoftSPI initialized: CS=%d, IRQ=%d", cs_pin_, config.touch_config.base.int_gpio_num);
    return;

err:
    if (tp) {
        free(tp);
        handle_ = nullptr;
    }
}

XPT2046_SoftSPI::~XPT2046_SoftSPI() {
    if (indev_) {
        lv_indev_delete(indev_);
    }
    if (handle_) {
        del(handle_);
    }
}

std::unique_ptr<XPT2046_SoftSPI> XPT2046_SoftSPI::create(const Config& config) {
    auto driver = std::unique_ptr<XPT2046_SoftSPI>(new XPT2046_SoftSPI(config));
    if (driver->init()) {
        return driver;
    }
    return nullptr;
}

bool XPT2046_SoftSPI::init() {
    spi_->begin();
    return true;
}

esp_err_t XPT2046_SoftSPI::del(esp_lcd_touch_handle_t tp) {
    esp_lcd_touch_xpt2046_t* xpt_tp = (esp_lcd_touch_xpt2046_t*)tp;
    if (xpt_tp->base.config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(xpt_tp->base.config.int_gpio_num);
    }
    free(xpt_tp);
    return ESP_OK;
}

esp_err_t XPT2046_SoftSPI::read_data(esp_lcd_touch_handle_t tp) {
    esp_lcd_touch_xpt2046_t* xpt_tp = (esp_lcd_touch_xpt2046_t*)tp;
    auto* driver = static_cast<XPT2046_SoftSPI*>(xpt_tp->user_data);
    uint16_t z1 = 0, z2 = 0;
    uint32_t x = 0, y = 0;
    uint8_t point_count = 0;

    if (xpt_tp->base.config.int_gpio_num != GPIO_NUM_NC && gpio_get_level(xpt_tp->base.config.int_gpio_num)) {
        xpt_tp->base.data.points = 0;
        ESP_LOGD(TAG, "No touch: IRQ high");
        return ESP_OK;
    }

    gpio_set_level(driver->cs_pin_, 0);
    ESP_RETURN_ON_ERROR(driver->read_register(Z_VALUE_1, &z1), TAG, "Read Z1 failed");
    ESP_RETURN_ON_ERROR(driver->read_register(Z_VALUE_2, &z2), TAG, "Read Z2 failed");
    uint16_t z = (z1 >> 3) + (XPT2046_ADC_LIMIT - (z2 >> 3));
    if (z < Z_THRESHOLD) {
        xpt_tp->base.data.points = 0;
        gpio_set_level(driver->cs_pin_, 1);
        ESP_LOGD(TAG, "No touch: z=%u below threshold", z);
        return ESP_OK;
    }

    uint16_t discard_buf = 0;
    ESP_RETURN_ON_ERROR(driver->read_register(X_POSITION, &discard_buf), TAG, "Read discard failed");

    constexpr uint8_t max_points = 4; // Match atanisoft
    for (uint8_t idx = 0; idx < max_points; idx++) {
        uint16_t x_temp = 0, y_temp = 0;
        ESP_RETURN_ON_ERROR(driver->read_register(X_POSITION, &x_temp), TAG, "Read X failed");
        ESP_RETURN_ON_ERROR(driver->read_register(Y_POSITION, &y_temp), TAG, "Read Y failed");
        x_temp >>= 3;
        y_temp >>= 3;
        if (x_temp >= 50 && x_temp <= XPT2046_ADC_LIMIT - 50 &&
            y_temp >= 50 && y_temp <= XPT2046_ADC_LIMIT - 50) {
            x += x_temp;
            y += y_temp;
            point_count++;
        }
    }
    gpio_set_level(driver->cs_pin_, 1);

    esp_lcd_touch_xpt2046_config_t* cfg = (esp_lcd_touch_xpt2046_config_t*)xpt_tp->base.config.user_data;
    if (point_count >= max_points / 2) {
        x /= point_count;
        y /= point_count;
        point_count = 1;
        int32_t x_scaled = (int32_t)x;
        int32_t y_scaled = (int32_t)y;
        if (cfg->x_max_raw != cfg->x_min_raw) {
            x_scaled = (x_scaled - cfg->x_min_raw) * cfg->base.x_max / (cfg->x_max_raw - cfg->x_min_raw);
        }
        if (cfg->y_max_raw != cfg->y_min_raw) {
            y_scaled = (y_scaled - cfg->y_min_raw) * cfg->base.y_max / (cfg->y_max_raw - cfg->y_min_raw);
        }
        x = x_scaled < 0 ? 0 : (x_scaled > cfg->base.x_max ? cfg->base.x_max : x_scaled);
        y = y_scaled < 0 ? 0 : (y_scaled > cfg->base.y_max ? cfg->base.y_max : y_scaled);
        if (cfg->swap_xy) {
            std::swap(x, y);
        }
        if (cfg->mirror_x) {
            x = cfg->base.x_max - x;
        }
        if (cfg->mirror_y) {
            y = cfg->base.y_max - y;
        }
    } else {
        z = point_count = 0;
    }

    xpt_tp->base.data.coords[0].x = x;
    xpt_tp->base.data.coords[0].y = y;
    xpt_tp->base.data.coords[0].strength = z;
    xpt_tp->base.data.points = point_count;
    ESP_LOGD(TAG, "Read: x=%" PRIu32 ", y=%" PRIu32 ", z=%u, points=%u", x, y, z, point_count);
    return ESP_OK;
}

bool XPT2046_SoftSPI::get_xy(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y,
                             uint16_t* strength, uint8_t* point_num, uint8_t max_point_num) {
    esp_lcd_touch_xpt2046_t* xpt_tp = (esp_lcd_touch_xpt2046_t*)tp;
    *point_num = std::min(xpt_tp->base.data.points, max_point_num);
    for (size_t i = 0; i < *point_num; i++) {
        x[i] = xpt_tp->base.data.coords[i].x;
        y[i] = xpt_tp->base.data.coords[i].y;
        if (strength) strength[i] = xpt_tp->base.data.coords[i].strength;
    }
    xpt_tp->base.data.points = 0;
    if (*point_num) {
        ESP_LOGI(TAG, "Touch point: x=%u, y=%u, strength=%u", x[0], y[0], strength ? strength[0] : 0);
    } else {
        ESP_LOGD(TAG, "No touch points");
    }
    return *point_num > 0;
}

void XPT2046_SoftSPI::lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* driver = static_cast<XPT2046_SoftSPI*>(lv_indev_get_user_data(indev));
    uint16_t x[1], y[1], strength[1];
    uint8_t points = 0;
    driver->read_data(driver->handle_);
    driver->get_xy(driver->handle_, x, y, strength, &points, 1);
    if (points) {
        data->point.x = x[0];
        data->point.y = y[0];
        data->state = LV_INDEV_STATE_PR;
        ESP_LOGI(TAG, "LVGL touch: x=%u, y=%u", x[0], y[0]);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

esp_err_t XPT2046_SoftSPI::read_register(uint8_t reg, uint16_t* value) {
    uint8_t buf[2] = {0, 0};
    spi_->cs_low();
    spi_->transfer(reg);
    ets_delay_us(5); // Reduced for faster transaction
    buf[0] = spi_->transfer(0x00);
    buf[1] = spi_->transfer(0x00);
    spi_->cs_high();
    *value = ((buf[0] << 8) | buf[1]);
    ESP_LOGD(TAG, "Read reg=0x%x, value=%u", reg, *value);
    return ESP_OK;
}

void XPT2046_SoftSPI::get_raw_touch(uint16_t& x, uint16_t& y) {
    uint16_t z1 = 0, z2 = 0;
    gpio_set_level(cs_pin_, 0);
    read_register(Z_VALUE_1, &z1);
    read_register(Z_VALUE_2, &z2);
    uint16_t z = (z1 >> 3) + (XPT2046_ADC_LIMIT - (z2 >> 3));
    if (z < Z_THRESHOLD) {
        x = y = 0;
        gpio_set_level(cs_pin_, 1);
        ESP_LOGD(TAG, "Raw touch: z=%u below threshold", z);
        return;
    }
    uint16_t discard_buf = 0;
    read_register(X_POSITION, &discard_buf);
    read_register(X_POSITION, &x);
    read_register(Y_POSITION, &y);
    gpio_set_level(cs_pin_, 1);
    x >>= 3;
    y >>= 3;
    if (x < 50 || x > XPT2046_ADC_LIMIT - 50 || y < 50 || y > XPT2046_ADC_LIMIT - 50) {
        x = y = 0;
        ESP_LOGD(TAG, "Raw touch: x=%u, y=%u out of bounds", x, y);
    } else {
        ESP_LOGD(TAG, "Raw touch: x=%u, y=%u, z=%u", x, y, z);
    }
}
