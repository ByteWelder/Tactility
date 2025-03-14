#include "TCA9534.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <rom/gpio.h>

#define I2C_MASTER_TIMEOUT_MS       1000

#define TCA9534_LIB_TAG "TCA9534"
#define TCA9534_IO_NUM 8

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TCA9534 Internal configuration and pin registers
 */
typedef enum {
    TCA9534_REG_INPUT_PORT,
    TCA9534_REG_OUTPUT_PORT,
    TCA9534_REG_POLARITY_INVERSION,
    TCA9534_REG_CONFIGURATION
} TCA9534_REGISTER;

/**
 * @brief Default TCA9534 interrupt task
 */
void TCA9534_default_interrupt_task(void * pvParameters){
    ESP_LOGW(TCA9534_LIB_TAG, "No interrupt task defined! Using standard TCA9523 interrupt task!");
    TCA9534_IO_EXP* io_exp = (TCA9534_IO_EXP*) pvParameters;
    uint32_t io_num;
    while(1){
        if(xTaskNotifyWait(0,0,&io_num,portTICK_PERIOD_MS) == pdTRUE) {
            uint8_t input_status = get_tca9534_all_io_pin_input_status(io_exp);
            printf("Current input status (pin : status):\n");
            for (uint8_t i = 0; i < TCA9534_IO_NUM; i++)
                printf("P%d : %d\n", i, (input_status & (1<<i)) == (1 << i));
        }
    }
}

/**
*  @brief TCA9534 interrupt handler
*/
static void  IRAM_ATTR TCA9534_interrupt_handler(void *args){
    TCA9534_IO_EXP* io_exp = (TCA9534_IO_EXP*) args;
    xTaskNotifyFromISR(*io_exp->interrupt_task, 0, eNoAction, 0);
}

/**
* @brief Setup TCA9534 interrupts
*/
void setup_tca9534_interrupt_handler(TCA9534_IO_EXP* io_exp){
    if(io_exp->interrupt_task == NULL){
        xTaskCreate(
                TCA9534_default_interrupt_task,       /* Function that implements the task. */
                "NAME",          /* Text name for the task. */
                2048,      /* Stack size in words, not bytes. */
                ( void * ) io_exp,    /* Parameter passed into the task. */
                10,/* Priority at which the task is created. */
                io_exp->interrupt_task);      /* Used to pass out the created task's handle. */

    }

    gpio_pad_select_gpio(GPIO_NUM_26);
    gpio_set_direction(GPIO_NUM_26,GPIO_MODE_INPUT);
    gpio_intr_enable(GPIO_NUM_26);

    gpio_set_intr_type(io_exp->interrupt_pin, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(io_exp->interrupt_pin, TCA9534_interrupt_handler, (void *)io_exp);
}

esp_err_t write_tca9534_reg(TCA9534_IO_EXP* io_exp, TCA9534_REGISTER cmd, uint8_t data) {
    uint8_t write_buffer[2] = {cmd, data};
    return i2c_master_write_to_device(io_exp->i2c_master_port, io_exp->I2C_ADDR, write_buffer,
                                      sizeof(write_buffer), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t read_tca9534_reg(TCA9534_IO_EXP* io_exp, TCA9534_REGISTER cmd, uint8_t *read_buff) {
    uint8_t reg = cmd;
    return i2c_master_write_read_device(io_exp->i2c_master_port, io_exp->I2C_ADDR, &reg,
                                        1, read_buff, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

int16_t get_tca9534_all_io_pin_input_status(TCA9534_IO_EXP* io_exp) {
    uint8_t result = 0;
    esp_err_t status = read_tca9534_reg(io_exp, TCA9534_REG_INPUT_PORT, &result);
    return (status == ESP_OK) ? result : TCA9534_ERROR;
}

int16_t get_io_pin_input_status(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin) {
    int16_t result = get_tca9534_all_io_pin_input_status(io_exp);
    if (result != TCA9534_ERROR)
        result &= (1 << io_pin);
    return (result == (1<< io_pin));
}

int16_t get_all_io_pin_direction(TCA9534_IO_EXP* io_exp) {
    uint8_t result;
    esp_err_t status = read_tca9534_reg(io_exp, TCA9534_REG_CONFIGURATION, &result);
    return (status == ESP_OK) ? result : TCA9534_ERROR;
}

int16_t get_io_pin_direction(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin) {
    int16_t result = get_all_io_pin_direction(io_exp);
    if (result != TCA9534_ERROR)
        result &= (1 << io_pin);
    return (result == (1<< io_pin));
}

int16_t get_all_io_polarity_inversion(TCA9534_IO_EXP* io_exp) {
    uint8_t result;
    esp_err_t status = read_tca9534_reg(io_exp, TCA9534_REG_POLARITY_INVERSION, &result);
    return (status == ESP_OK) ? result : TCA9534_ERROR;
}

int16_t get_io_pin_polarity_inversion(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin) {
    int16_t result = get_all_io_polarity_inversion(io_exp);
    if (result != TCA9534_ERROR)
        result &= (1 << io_pin);
    return (result == (1<< io_pin));
}

esp_err_t set_all_tca9534_io_pins_direction(TCA9534_IO_EXP* io_exp, TCA9534_PORT_DIRECTION properties) {
    uint8_t dir = (properties == TCA9534_OUTPUT) ? 0x00 : 0xFF;
    esp_err_t status = write_tca9534_reg(io_exp, TCA9534_REG_CONFIGURATION, dir);
    return status;
}

esp_err_t set_tca9534_io_pin_direction(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin, TCA9534_PORT_DIRECTION properties) {
    uint8_t port_status = 0;
    esp_err_t status = read_tca9534_reg(io_exp, TCA9534_REG_CONFIGURATION, &port_status);
    port_status = (properties != TCA9534_OUTPUT) ? (port_status | (1 << io_pin)) : (port_status & ~(1 << io_pin));

    status |= write_tca9534_reg(io_exp, TCA9534_REG_CONFIGURATION, port_status);
    return status;
}

esp_err_t set_tca9534_io_pin_output_state(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin, uint8_t state) {
    uint8_t port_status = 0;
    esp_err_t status = read_tca9534_reg(io_exp, TCA9534_REG_OUTPUT_PORT, &port_status);
    port_status = (state != 0) ? (port_status | (1 << io_pin)) : (port_status & ~(1 << io_pin));

    status |= write_tca9534_reg(io_exp, TCA9534_REG_OUTPUT_PORT, port_status);
    return status;
}

#ifdef __cplusplus
}
#endif
