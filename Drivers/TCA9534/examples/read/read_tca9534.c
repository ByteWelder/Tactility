#include "esp_log.h"
#include "driver/i2c.h"
#include "TCA9534.h"


#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */


/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(i2c_config_t *conf) {
    int i2c_master_port = I2C_MASTER_NUM;

    conf->mode = I2C_MODE_MASTER;
    conf->master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf->sda_io_num = I2C_MASTER_SDA_IO;
    conf->scl_io_num = I2C_MASTER_SCL_IO;
    conf->sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf->scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_param_config(i2c_master_port, conf);

    return i2c_driver_install(i2c_master_port, conf->mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


static const char *TAG = "TCA9534-Example";

void app_main(void) {
    TCA9534_IO_EXP IO_EXP1;
    esp_err_t status = i2c_master_init(&IO_EXP1.i2c_conf);
    if (status == ESP_OK) {
        ESP_LOGI(TAG, "I2C initialized successfully");
        IO_EXP1.I2C_ADDR = 0b0100000;
        IO_EXP1.i2c_master_port = I2C_MASTER_NUM;

        set_tca9534_io_pin_direction(IO_EXP1, TCA9534_IO0, TCA9534_INPUT);
        set_tca9534_io_pin_direction(IO_EXP1, TCA9534_IO1, TCA9534_OUTPUT);

        int pin_state = 0;
        while (1) {
            pin_state = get_io_pin_input_status(IO_EXP1, TCA9534_IO0);
            if (pin_state == -1) {
                ESP_LOGE(TAG, "Cannot get pin status from TCA9534");
                break;
            }
            set_tca9534_io_pin_output_state(IO_EXP1, TCA9534_IO1, pin_state);
            vTaskDelay(100 / portTICK_RATE_MS);
        }

        ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
        ESP_LOGI(TAG, "I2C unitialized successfully");
    }
}