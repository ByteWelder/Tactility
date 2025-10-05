#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Logical GPIO pin identifier used by the HAL. Typically maps to the SoC GPIO number. */
typedef unsigned int GpioPin;
/** Value indicating that no GPIO pin is used/applicable. */
#define GPIO_NO_PIN -1

/** GPIO pin mode used by the HAL.
 * @warning The order must match tt::hal::gpio::Mode
 */
enum GpioMode {
    /** Pin is disabled (high-impedance). */
    GpioModeDisable = 0,
    /** Pin configured as input only. */
    GpioModeInput,
    /** Pin configured as push-pull output only. */
    GpioModeOutput,
    /** Pin configured as open-drain output only. */
    GpioModeOutputOpenDrain,
    /** Pin configured for both input and output (push-pull). */
    GpioModeInputOutput,
    /** Pin configured for both input and output (open-drain). */
    GpioModeInputOutputOpenDrain
};

/** Configure a single GPIO pin.
 * @param[in] pin      GPIO number to configure.
 * @param[in] mode     Desired I/O mode for the pin.
 * @param[in] pullUp   Enable internal pull-up if true.
 * @param[in] pullDown Enable internal pull-down if true.
 * @return true on success, false if the pin is invalid or configuration failed.
 */
bool tt_hal_gpio_configure(GpioPin pin, GpioMode mode, bool pullUp, bool pullDown);

/** Configure a set of GPIO pins in one call.
 * The bit index of pin N is (1ULL << N).
 * @param[in] pinBitMask Bit mask of pins to configure.
 * @param[in] mode       Desired I/O mode for the pins.
 * @param[in] pullUp     Enable internal pull-up on the selected pins if true.
 * @param[in] pullDown   Enable internal pull-down on the selected pins if true.
 * @return true on success, false if any pin is invalid or configuration failed.
 */
bool tt_hal_gpio_configure_with_pin_bitmask(uint64_t pinBitMask, GpioMode mode, bool pullUp, bool pullDown);

/** Set the input/output mode for the specified pin.
 * @param[in] pin  The pin to configure.
 * @param[in] mode The mode to set.
 * @return true on success, false if the pin is invalid or mode not supported.
 */
bool tt_hal_gpio_set_mode(GpioPin pin, GpioMode mode);

/** Read the current logic level of a pin.
 * The pin should be configured for input or input/output.
 * @param[in] pin The pin to read.
 * @return true if the level is high, false if low. If the pin is invalid, the
 *         behavior is implementation-defined and may return false.
 */
bool tt_hal_gpio_get_level(GpioPin pin);

/** Drive the output level of a pin.
 * The pin should be configured for output or input/output.
 * @param[in] pin   The pin to drive.
 * @param[in] level Output level to set (true = high, false = low).
 * @return true on success, false if the pin is invalid or not configured as output.
 */
bool tt_hal_gpio_set_level(GpioPin pin, bool level);

/** Get the number of GPIO pins available on this platform.
 * @return The count of valid GPIO pins.
 */
int tt_hal_gpio_get_pin_count();

#ifdef __cplusplus
}
#endif
