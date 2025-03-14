#ifndef TCA9534_IDF_TCA9534_H
#define TCA9534_IDF_TCA9534_H

#include <driver/i2c.h>
#include <driver/gpio.h>

#define TCA9534_ERROR -1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TCA9534 IO Pins mapping
 */
typedef enum {
    TCA9534_IO0,
    TCA9534_IO1,
    TCA9534_IO2,
    TCA9534_IO3,
    TCA9534_IO4,
    TCA9534_IO5,
    TCA9534_IO6,
    TCA9534_IO7
} TCA9534_PINS;

/**
 * @brief TCA9534 Port direction parameters
 */
typedef enum {
    TCA9534_OUTPUT,
    TCA9534_INPUT
} TCA9534_PORT_DIRECTION;

/**
 * @brief TCA9534 initialization parameters
 */
typedef struct {
    uint8_t I2C_ADDR;
    int i2c_master_port;
    //Only when mode is set to interrupt, otherwise it won't be used..
    gpio_num_t interrupt_pin;
    TaskHandle_t* interrupt_task;
} TCA9534_IO_EXP;

/**
 * @brief Setup interrupts using the builtin IO_EXP_INT pin of the tca9534 and interrupt handler+task
 * @param io_exp which contains the gpio pin where IO_EXP_INT is connected(interrupt_pin)
 *               And optionally contains the task to run when interrupt triggered (interrupt_task) if not defined the
 *               default handler will be used.
 */
void setup_tca9534_interrupt_handler(TCA9534_IO_EXP* io_exp);

/**
 * @brief Get the current input state of the specified input pin (1 or 0)
 *
 * @param io_exp The io expander instance to read from or write to
 * @param io_pin The io expander pin to read the state from
 *
 * @return
 *     - 0   Success! Pin is Low
 *     - 1   Success! Pin is High
 *     - TCA9534_ERROR(-1)  Error!   Something went wrong in the process of reading the io expander
 */
int16_t get_io_pin_input_status(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin);

/**
 * @brief Get the current input state of all the io expander pins
 *
 * @param io_exp The io expander instance to read from or write to
 *
 * @return
 *     - 0x00 - 0xFF  Success! Dump of input register, 1 bit is equal to 1 of the physical pins (Lower 8 bits of 16 bits result)
 *     - TCA9534_ERROR(-1)  Error!   Something went wrong in the process of reading the io expander
 */
int16_t get_tca9534_all_io_pin_input_status(TCA9534_IO_EXP* io_exp);

/**
 * @brief Get the current direction of all the io expander pins
 *
 * @param io_exp The io expander instance to read from or write to
 *
 * @return
 *     - 0x00 - 0xFF  Success! Dump of configuration register, 1 bit is equal to 1 of the physical pins (Lower 8 bits of 16 bits result)
 *     - TCA9534_ERROR(-1)  Error!   Something went wrong in the process of reading the io expander
 */
int16_t get_all_io_pin_direction(TCA9534_IO_EXP* io_exp);

/**
 * @brief Get the current polarity inversion state of all the io expander pins
 *
 * @param io_exp The io expander instance to read from or write to
 *
 * @return
 *     - 0x00 - 0xFF  Success! Dump of configuration register, 1 bit is equal to 1 of the physical pins (Lower 8 bits of 16 bits result)
 *     - TCA9534_ERROR(-1)  Error!   Something went wrong in the process of reading the io expander
 */
int16_t get_all_io_polarity_inversion(TCA9534_IO_EXP* io_exp);

/**
 * @brief Get the current direction of the specified io expander pin
 *
 * @param io_exp The io expander instance to read from or write to
 * @param io_pin The io expander pin to read polarity inversion from
 *
 * @return
 *     - 0   Success! Pin is Not inverted
 *     - 1   Success! Pin is Inverted
 *     - TCA9534_ERROR(-1)  Error!   Something went wrong in the process of reading the io expander
 */
int16_t get_io_pin_polarity_inversion(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin);

/**
 * @brief Get the current direction of the specified physical pin (0 means OUTPUT or 1 means INPUT)
 *
 * @param io_exp The io expander instance to read from or write to
 * @param io_pin The io expander pin to read the state from
 *
 * @return
 *     - 0   Success! Pin is OUTPUT
 *     - 1   Success! Pin is INPUT
 *     - TCA9534_ERROR(-1)  Error!   Something went wrong in the process of reading the io expander
 */
int16_t get_io_pin_direction(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin);

/**
 * @brief Sets all physical pins of the io expander to a specified direction (INPUT or OUTPUT)
 *
 * @param io_exp The io expander instance to read from or write to
 * @param properties The pin direction to be set (INPUT or OUTPUT)
 *
 * @return
 *     - ESP_OK  Success!
 *     - ESP_ERR Error!
 */
esp_err_t set_all_tca9534_io_pins_direction(TCA9534_IO_EXP* io_exp, TCA9534_PORT_DIRECTION properties);

/**
 * @brief Set physical pin of the io expander to a specified direction (INPUT or OUTPUT)
 *
 * @param io_exp The io expander instance to read from or write to
 * @param io_pin The io expander physical pin to be set
 * @param properties The pin direction to be set (INPUT or OUTPUT)
 *
 * @return
 *     - ESP_OK  Success!
 *     - ESP_ERR Error!
 */
esp_err_t set_tca9534_io_pin_direction(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin, TCA9534_PORT_DIRECTION properties);

/**
 * @brief Set physical pin of the io expander to an specified output state (HIGH(1) or LOW(0))
 *
 * @param io_exp The io expander instance to read from or write to
 * @param io_pin The io expander physical pin to be set
 * @param state The pin state to be set (1 or 0)
 *
 * @return
 *     - ESP_OK  Success!
 *     - ESP_ERR Error!
 *
 * @note Pin output state can be inverted with the inversion register
 */
esp_err_t set_tca9534_io_pin_output_state(TCA9534_IO_EXP* io_exp, TCA9534_PINS io_pin, uint8_t state);

#ifdef __cplusplus
}
#endif

#endif //TCA9534_IDF_TCA9534_H
