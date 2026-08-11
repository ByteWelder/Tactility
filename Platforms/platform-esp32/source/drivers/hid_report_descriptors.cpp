// SPDX-License-Identifier: Apache-2.0
#include <tactility/drivers/hid_report_descriptors.h>

extern "C" {

// Keyboard (report ID 1) + Consumer (report ID 2)
const uint8_t hid_report_map_keyboard_consumer[] = {
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x06,        // Usage: Keyboard
    0xA1, 0x01,        // Collection: Application
    // --- Keyboard
    0x85, 0x01,        // Report ID: 1
    0x05, 0x07,        // Usage Page: Keyboard/Keypad
    0x19, 0xE0,        // Usage Minimum: Left Control
    0x29, 0xE7,        // Usage Maximum: Right GUI
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x01,        // Logical Maximum: 1
    0x75, 0x01,        // Report Size: 1
    0x95, 0x08,        // Report Count: 8
    0x81, 0x02,        // Input: Data, Variable, Absolute (modifier byte)
    0x75, 0x08,        // Report Size: 8
    0x95, 0x01,        // Report Count: 1
    0x81, 0x01,        // Input: Constant, Array, Absolute (reserved byte)
    0x05, 0x08,        // Usage Page: LEDs
    0x19, 0x01,        // Usage Minimum: 0x01
    0x29, 0x05,        // Usage Maximum: 0x05
    0x75, 0x01,        // Report Size: 1
    0x95, 0x05,        // Report Count: 5
    0x91, 0x02,        // Output: Data, Variable, Absolute (LED state)
    0x75, 0x03,        // Report Size: 3
    0x95, 0x01,        // Report Count: 1
    0x91, 0x01,        // Output: Constant, Array, Absolute (LED padding)
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x73,        // Logical Maximum: 115
    0x05, 0x07,        // Usage Page: Keyboard/Keypad
    0x19, 0x00,        // Usage Minimum: 0x00
    0x29, 0x73,        // Usage Maximum: 0x73
    0x75, 0x08,        // Report Size: 8
    0x95, 0x06,        // Report Count: 6
    0x81, 0x00,        // Input: Data, Array, Absolute (keycode[6])
    0xC0,              // End Collection
    // --- Consumer / Media Keys
    0x05, 0x0C,        // Usage Page: Consumer
    0x09, 0x01,        // Usage: Consumer Control
    0xA1, 0x01,        // Collection: Application
    0x85, 0x02,        // Report ID: 2
    0x15, 0x00,        // Logical Minimum: 0
    0x26, 0xFF, 0x03,  // Logical Maximum: 1023
    0x19, 0x00,        // Usage Minimum: 0x00
    0x2A, 0xFF, 0x03,  // Usage Maximum: 0x3FF
    0x75, 0x10,        // Report Size: 16
    0x95, 0x01,        // Report Count: 1
    0x81, 0x00,        // Input: Data, Array, Absolute
    0xC0,              // End Collection
};
const size_t hid_report_map_keyboard_consumer_len = sizeof(hid_report_map_keyboard_consumer);

// Mouse only (report ID 1)
const uint8_t hid_report_map_mouse[] = {
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x02,        // Usage: Mouse
    0xA1, 0x01,        // Collection: Application
    0x85, 0x01,        // Report ID: 1
    0x09, 0x01,        // Usage: Pointer
    0xA1, 0x00,        // Collection: Physical
    0x05, 0x09,        // Usage Page: Button
    0x19, 0x01,        // Usage Minimum: 1
    0x29, 0x03,        // Usage Maximum: 3
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x01,        // Logical Maximum: 1
    0x95, 0x03,        // Report Count: 3
    0x75, 0x01,        // Report Size: 1
    0x81, 0x02,        // Input: Data, Variable, Absolute (3 buttons)
    0x95, 0x01,        // Report Count: 1
    0x75, 0x05,        // Report Size: 5
    0x81, 0x01,        // Input: Constant, Array, Absolute (padding to byte boundary)
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x30,        // Usage: X
    0x09, 0x31,        // Usage: Y
    0x09, 0x38,        // Usage: Wheel
    0x15, 0x81,        // Logical Minimum: -127
    0x25, 0x7F,        // Logical Maximum: 127
    0x75, 0x08,        // Report Size: 8
    0x95, 0x03,        // Report Count: 3
    0x81, 0x06,        // Input: Data, Variable, Relative
    0xC0,              // End Collection (Physical)
    0xC0,              // End Collection (Application)
};
const size_t hid_report_map_mouse_len = sizeof(hid_report_map_mouse);

// Keyboard (report ID 1) + Consumer (report ID 2) + Mouse (report ID 3)
const uint8_t hid_report_map_keyboard_consumer_mouse[] = {
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x06,        // Usage: Keyboard
    0xA1, 0x01,        // Collection: Application
    // --- Keyboard
    0x85, 0x01,        // Report ID: 1
    0x05, 0x07,        // Usage Page: Keyboard/Keypad
    0x19, 0xE0,        // Usage Minimum: Left Control
    0x29, 0xE7,        // Usage Maximum: Right GUI
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x01,        // Logical Maximum: 1
    0x75, 0x01,        // Report Size: 1
    0x95, 0x08,        // Report Count: 8
    0x81, 0x02,        // Input: Data, Variable, Absolute (modifier byte)
    0x75, 0x08,        // Report Size: 8
    0x95, 0x01,        // Report Count: 1
    0x81, 0x01,        // Input: Constant, Array, Absolute (reserved byte)
    0x05, 0x08,        // Usage Page: LEDs
    0x19, 0x01,        // Usage Minimum: 0x01
    0x29, 0x05,        // Usage Maximum: 0x05
    0x75, 0x01,        // Report Size: 1
    0x95, 0x05,        // Report Count: 5
    0x91, 0x02,        // Output: Data, Variable, Absolute (LED state)
    0x75, 0x03,        // Report Size: 3
    0x95, 0x01,        // Report Count: 1
    0x91, 0x01,        // Output: Constant, Array, Absolute (LED padding)
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x73,        // Logical Maximum: 115
    0x05, 0x07,        // Usage Page: Keyboard/Keypad
    0x19, 0x00,        // Usage Minimum: 0x00
    0x29, 0x73,        // Usage Maximum: 0x73
    0x75, 0x08,        // Report Size: 8
    0x95, 0x06,        // Report Count: 6
    0x81, 0x00,        // Input: Data, Array, Absolute (keycode[6])
    0xC0,              // End Collection
    // --- Consumer / Media Keys
    0x05, 0x0C,        // Usage Page: Consumer
    0x09, 0x01,        // Usage: Consumer Control
    0xA1, 0x01,        // Collection: Application
    0x85, 0x02,        // Report ID: 2
    0x15, 0x00,        // Logical Minimum: 0
    0x26, 0xFF, 0x03,  // Logical Maximum: 1023
    0x19, 0x00,        // Usage Minimum: 0x00
    0x2A, 0xFF, 0x03,  // Usage Maximum: 0x3FF
    0x75, 0x10,        // Report Size: 16
    0x95, 0x01,        // Report Count: 1
    0x81, 0x00,        // Input: Data, Array, Absolute
    0xC0,              // End Collection
    // --- Mouse
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x02,        // Usage: Mouse
    0xA1, 0x01,        // Collection: Application
    0x85, 0x03,        // Report ID: 3
    0x09, 0x01,        // Usage: Pointer
    0xA1, 0x00,        // Collection: Physical
    0x05, 0x09,        // Usage Page: Button
    0x19, 0x01,        // Usage Minimum: 1
    0x29, 0x03,        // Usage Maximum: 3
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x01,        // Logical Maximum: 1
    0x95, 0x03,        // Report Count: 3
    0x75, 0x01,        // Report Size: 1
    0x81, 0x02,        // Input: Data, Variable, Absolute (3 buttons)
    0x95, 0x01,        // Report Count: 1
    0x75, 0x05,        // Report Size: 5
    0x81, 0x01,        // Input: Constant, Array, Absolute (padding to byte boundary)
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x30,        // Usage: X
    0x09, 0x31,        // Usage: Y
    0x09, 0x38,        // Usage: Wheel
    0x15, 0x81,        // Logical Minimum: -127
    0x25, 0x7F,        // Logical Maximum: 127
    0x75, 0x08,        // Report Size: 8
    0x95, 0x03,        // Report Count: 3
    0x81, 0x06,        // Input: Data, Variable, Relative
    0xC0,              // End Collection (Physical)
    0xC0,              // End Collection (Application)
};
const size_t hid_report_map_keyboard_consumer_mouse_len = sizeof(hid_report_map_keyboard_consumer_mouse);

// Gamepad only (report ID 1):
// left stick = X/Y, right stick = Rx/Ry, triggers share a single Z axis (LT = positive, RT = negative, centered = 0,),
// hat/dpad, 10 buttons (1=A 2=B 3=X 4=Y 5=LB 6=RB 7=Back/Select 8=Start 9=L3 10=R3).
// DirectInput assigns axis usages to lX/lY/lZ/lRx/lRy by USAGE TAG, not by declaration order, so X/Y/Rx/Ry/Z in that order (not X/Y/Z/Rx/Ry) is correct.
const uint8_t hid_report_map_gamepad[] = {
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x05,        // Usage: Game Pad
    0xA1, 0x01,        // Collection: Application
    0x85, 0x01,        // Report ID: 1
    // --- 8-bit X, Y (left stick), Rx, Ry (right stick), Z (triggers, LT=+, RT=-) (min -127, max 127)
    0x05, 0x01,        // Usage Page: Generic Desktop
    0x09, 0x30,        // Usage: X
    0x09, 0x31,        // Usage: Y
    0x09, 0x33,        // Usage: Rx
    0x09, 0x34,        // Usage: Ry
    0x09, 0x32,        // Usage: Z
    0x15, 0x81,        // Logical Minimum: -127
    0x25, 0x7F,        // Logical Maximum: 127
    0x75, 0x08,        // Report Size: 8
    0x95, 0x05,        // Report Count: 5
    0x81, 0x02,        // Input: Data, Variable, Absolute
    // --- 4-bit hat/dpad (0-7 = direction clockwise from Up, 8 = centered/released via Null State).
    // Logical range must be 0-7 (not 1-8) and size must be a nibble (not a full byte) - both are required for DirectInput to recognize this as a POV.
    0x09, 0x39,        // Usage: Hat Switch
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x07,        // Logical Maximum: 7
    0x35, 0x00,        // Physical Minimum: 0
    0x46, 0x3B, 0x01,  // Physical Maximum: 315
    0x65, 0x14,        // Unit: Eng Rot:Angular Pos (degrees)
    0x75, 0x04,        // Report Size: 4
    0x95, 0x01,        // Report Count: 1
    0x81, 0x42,        // Input: Data, Variable, Absolute, Null State (value 8 = centered/released, outside the declared 0-7 logical range)
    0x65, 0x00,        // Unit: None - resets the Unit global item so it doesn't leak onto the button/padding items below (Unit is global, not local, in HID).
    0x75, 0x04,        // Report Size: 4
    0x95, 0x01,        // Report Count: 1
    0x81, 0x03,        // Input: Constant, Variable, Absolute (pad hat nibble to a full byte)
    // --- 10 one-bit buttons: 1=A 2=B 3=X 4=Y 5=LB 6=RB 7=Back/Select 8=Start 9=L3 10=R3
    0x05, 0x09,        // Usage Page: Button
    0x19, 0x01,        // Usage Minimum: 1
    0x29, 0x0A,        // Usage Maximum: 10
    0x15, 0x00,        // Logical Minimum: 0
    0x25, 0x01,        // Logical Maximum: 1
    0x75, 0x01,        // Report Size: 1
    0x95, 0x0A,        // Report Count: 10
    0x81, 0x02,        // Input: Data, Variable, Absolute
    // --- 6-bit padding to byte-align the buttons field (10 bits -> 16 bits / 2 bytes)
    0x75, 0x06,        // Report Size: 6
    0x95, 0x01,        // Report Count: 1
    0x81, 0x03,        // Input: Constant, Variable, Absolute
    0xC0,              // End Collection
};
const size_t hid_report_map_gamepad_len = sizeof(hid_report_map_gamepad);

} // extern "C"
