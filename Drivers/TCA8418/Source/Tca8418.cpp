#include "Tca8418.h"
#include <Tactility/Log.h>

constexpr auto TAG = "TCA8418";

namespace registers {
static const uint8_t CFG = 0x01U;
static const uint8_t KP_GPIO1 = 0x1DU;
static const uint8_t KP_GPIO2 = 0x1EU;
static const uint8_t KP_GPIO3 = 0x1FU;

static const uint8_t KEY_EVENT_A = 0x04U;
static const uint8_t KEY_EVENT_B = 0x05U;
static const uint8_t KEY_EVENT_C = 0x06U;
static const uint8_t KEY_EVENT_D = 0x07U;
static const uint8_t KEY_EVENT_E = 0x08U;
static const uint8_t KEY_EVENT_F = 0x09U;
static const uint8_t KEY_EVENT_G = 0x0AU;
static const uint8_t KEY_EVENT_H = 0x0BU;
static const uint8_t KEY_EVENT_I = 0x0CU;
static const uint8_t KEY_EVENT_J = 0x0DU;
} // namespace registers


/** From https://github.com/adafruit/Adafruit_TCA8418/blob/main/Adafruit_TCA8418.cpp */
bool Tca8418::initMatrix(uint8_t rows, uint8_t columns) {
    if ((rows > 8) || (columns > 10))
        return false;

    if ((rows != 0) && (columns != 0)) {
        // Configure the keypad matrix.
        uint8_t mask = 0x00;
        for (int r = 0; r < rows; r++) {
            mask <<= 1;
            mask |= 1;
        }
        writeRegister(registers::KP_GPIO1, &mask, 1);

        mask = 0x00;
        for (int c = 0; c < columns && c < 8; c++) {
            mask <<= 1;
            mask |= 1;
        }
        writeRegister(registers::KP_GPIO2, &mask, 1);

        if (columns > 8) {
            if (columns == 9)
                mask = 0x01;
            else
                mask = 0x03;
            writeRegister(registers::KP_GPIO3, &mask, 1);
        }
    }

    return true;
}
void Tca8418::init(uint8_t numrows, uint8_t numcols) {
    /*
     *   | ADDRESS | REGISTER NAME | REGISTER DESCRIPTION | BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0 |
     *   |---------+---------------+----------------------+------+------+------+------+------+------+------+------|
     *   |    0x1D | KP_GPIO1      | Keypad/GPIO Select 1 | ROW7 | ROW6 | ROW5 | ROW4 | ROW3 | ROW2 | ROW1 | ROW0 |
     *   |    0x1E | KP_GPIO2      | Keypad/GPIO Select 2 | COL7 | COL6 | COL5 | COL4 | COL3 | COL2 | COL1 | COL0 |
     *   |    0x1F | KP_GPIO3      | Keypad/GPIO Select 3 | N/A  | N/A  | N/A  | N/A  | N/A  | N/A  | COL9 | COL8 |
     */

    num_rows = numrows;
    num_cols = numcols;

    initMatrix(num_rows, num_cols);

    /*
     *   BIT: NAME
     *
     *   7: AI
     *   Auto-increment for read and write operations; See below table for more information
     *   0 = disabled
     *   1 = enabled
     *
     *   6: GPI_E_CFG
     *   GPI event mode configuration
     *   0 = GPI events are tracked when keypad is locked
     *   1 = GPI events are not tracked when keypad is locked
     *
     *   5: OVR_FLOW_M
     *   Overflow mode
     *   0 = disabled; Overflow data is lost
     *   1 = enabled; Overflow data shifts with last event pushing first event out
     *
     *   4: INT_CFG
     *   Interrupt configuration
     *   0 = processor interrupt remains asserted (or low) if host tries to clear interrupt while there is
     *   still a pending key press, key release or GPI interrupt
     *   1 = processor interrupt is deasserted for 50 μs and reassert with pending interrupts
     *
     *   3: OVR_FLOW_IEN
     *   Overflow interrupt enable
     *   0 = disabled; INT is not asserted if the FIFO overflows
     *   1 = enabled; INT becomes asserted if the FIFO overflows
     *
     *   2: K_LCK_IEN
     *   Keypad lock interrupt enable
     *   0 = disabled; INT is not asserted after a correct unlock key sequence
     *   1 = enabled; INT becomes asserted after a correct unlock key sequence
     *
     *   1: GPI_IEN
     *   GPI interrupt enable to host processor
     *   0 = disabled; INT is not asserted for a change on a GPI
     *   1 = enabled; INT becomes asserted for a change on a GPI
     *
     *   0: KE_IEN
     *   Key events interrupt enable to host processor
     *   0 = disabled; INT is not asserted when a key event occurs
     *   1 = enabled; INT becomes asserted when a key event occurs
     */

    // 10111001 xB9 -- fifo overflow enabled
    // 10011001 x99 -- fifo overflow disabled
    writeRegister8(registers::CFG, 0x99);

    clear_released_list();
    clear_pressed_list();
}

bool Tca8418::update() {
    last_update_micros = this_update_micros;
    uint8_t key_down, key_event, key_row, key_col;

    key_event = get_key_event();
    // TODO: read gpio R7/R6 status? 0x14 bits 7&6
    // read(0x14, &new_keycode)

    // TODO: use tick function to get an update delta time
    this_update_micros = 0;
    delta_micros = this_update_micros - last_update_micros;

    if (key_event > 0) {
        key_down = (key_event & 0x80);
        uint16_t buffer = key_event;
        buffer &= 0x7F;
        buffer--;
        key_row = buffer / 10;
        key_col = buffer % 10;

        // always clear the released list
        clear_released_list();

        if (key_down) {
            add_pressed_key(key_row, key_col);
            // TODO reject ghosts (assume multiple key presses with the same hold time are ghosts.)

        } else {
            add_released_key(key_row, key_col);
            remove_pressed_key(key_row, key_col);
        }

        return true;
    }

    // Increment hold times for pressed keys
    for (int i = 0; i < pressed_key_count; i++) {
        pressed_list[i].hold_time += delta_micros;
    }

    return false;
}


void Tca8418::add_pressed_key(uint8_t row, uint8_t col) {
    if (pressed_key_count >= KEY_EVENT_LIST_SIZE)
        return;

    pressed_list[pressed_key_count].row = row;
    pressed_list[pressed_key_count].col = col;
    pressed_list[pressed_key_count].hold_time = 0;
    pressed_key_count++;
}

void Tca8418::add_released_key(uint8_t row, uint8_t col) {
    if (released_key_count >= KEY_EVENT_LIST_SIZE)
        return;

    released_key_count++;
    released_list[0].row = row;
    released_list[0].col = col;
}

void Tca8418::remove_pressed_key(uint8_t row, uint8_t col) {
    if (pressed_key_count == 0)
        return;

    // delete the pressed key
    for (int i = 0; i < pressed_key_count; i++) {
        if (pressed_list[i].row == row &&
            pressed_list[i].col == col) {
            // shift remaining keys left one index
            for (int j = i; i < pressed_key_count; j++) {
                if (j == KEY_EVENT_LIST_SIZE - 1)
                    break;
                pressed_list[j].row = pressed_list[j + 1].row;
                pressed_list[j].col = pressed_list[j + 1].col;
                pressed_list[j].hold_time = pressed_list[j + 1].hold_time;
            }
            break;
        }
    }
    pressed_key_count--;
}

void Tca8418::clear_pressed_list() {
    for (int i = 0; i < KEY_EVENT_LIST_SIZE; i++) {
        pressed_list[i].row = 255;
        pressed_list[i].col = 255;
    }
    pressed_key_count = 0;
}

void Tca8418::clear_released_list() {
    for (int i = 0; i < KEY_EVENT_LIST_SIZE; i++) {
        released_list[i].row = 255;
        released_list[i].col = 255;
    }
    released_key_count = 0;
}

uint8_t Tca8418::get_key_event() {
    uint8_t new_keycode = 0;

    readRegister8(registers::KEY_EVENT_A, new_keycode);
    return new_keycode;
}
