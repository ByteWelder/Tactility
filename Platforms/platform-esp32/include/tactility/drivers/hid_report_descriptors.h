// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Shared HID report descriptor tables ----
//
// HID report descriptors are transport-independent (the same USB HID Usage Tables spec applies
// regardless of whether the bytes travel over USB or BLE GATT), so these tables are shared
// between esp32_usb_hid_device.cpp and esp32_ble_hid.cpp rather than each keeping its own copy.
// Report ID scheme matches BtHidDeviceMode/UsbHidDeviceMode 1:1 across both transports:
//   Keyboard mode:          report ID 1 = keyboard, report ID 2 = consumer control
//   Mouse mode:              report ID 1 = mouse
//   Keyboard+Mouse mode:     report ID 1 = keyboard, 2 = consumer, 3 = mouse
//   Gamepad mode:            report ID 1 = gamepad

/** Keyboard (report ID 1, 8 bytes: modifier, reserved, keycode[6]) + Consumer (report ID 2,
 *  16-bit usage code, 2 bytes). Keyboard collection uses the standard boot-protocol byte
 *  layout, though the descriptor itself is Report protocol (report-ID-multiplexed with
 *  Consumer, which boot protocol can't express). */
extern const uint8_t hid_report_map_keyboard_consumer[];
extern const size_t hid_report_map_keyboard_consumer_len;

/** Mouse only (report ID 1, 4 bytes: buttons[3]+5 padding bits, X, Y, wheel). */
extern const uint8_t hid_report_map_mouse[];
extern const size_t hid_report_map_mouse_len;

/** Keyboard (report ID 1) + Consumer (report ID 2) + Mouse (report ID 3) combined. */
extern const uint8_t hid_report_map_keyboard_consumer_mouse[];
extern const size_t hid_report_map_keyboard_consumer_mouse_len;

/** Gamepad only (report ID 1, 8 bytes). Xbox-360-style layout: X/Y (1 byte each, left stick),
 *  Rx/Ry (1 byte each, right stick), Z (1 byte, triggers - L=positive, R=negative, centered=0),
 *  hat/dpad (1 byte, 1-8 = 8-direction clockwise from Up, 0 = centered/released), buttons[10]
 *  (2 bytes, 1=A 2=B 3=X 4=Y 5=LB 6=RB 7=Select 8=Start 9=L3 10=R3, 6 padding bits at the end). */
extern const uint8_t hid_report_map_gamepad[];
extern const size_t hid_report_map_gamepad_len;

#ifdef __cplusplus
}
#endif
