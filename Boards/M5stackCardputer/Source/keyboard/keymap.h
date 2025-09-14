/**
 * This file is part of esp32s3-keyboard.
 *
 * Copyright (C) 2020-2021 Yuquan He <heyuquan20b at ict dot ac dot cn>
 * (Institute of Computing Technology, Chinese Academy of Sciences)
 *
 * esp32s3-keyboard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * esp32s3-keyboard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with esp32s3-keyboard. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * USB HID format: 8 bytes
 * Byte 0: Keyboard modifier bits (SHIFT, ALT, CTRL etc)
 * Byte 1: reserved
 * Byte 2-7: Up to six keyboard usage indexes representing the keys that are
 *           currently "pressed". Order is not important, a key is either
 *           pressed (present in the buffer) or not pressed.
 */

#ifndef MY_KEYMAP_H
#define MY_KEYMAP_H

#include <stdint.h>

/**
 * Modifier masks - used for the first byte in the HID report.
 * NOTE: The second byte in the report is reserved, 0x00
 */
#define KEY_MOD_LCTRL  0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT   0x04
#define KEY_MOD_LMETA  0x08
#define KEY_MOD_RCTRL  0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT   0x40
#define KEY_MOD_RMETA  0x80

/**
 * Scan codes - last N slots in the HID report (usually 6).
 * 0x00 if no key pressed.
 *
 * If more than N keys are pressed, the HID reports
 * KEY_ERR_OVF in all slots to indicate this condition.
 */

#define KEY_NONE 0x00  // No key pressed
#define KEY_ERR_OVF \
    0x01  //  Keyboard Error Roll Over - used for all slots if too many keys are
          //  pressed ("Phantom key")
// 0x02 //  Keyboard POST Fail
// 0x03 //  Keyboard Error Undefined
#define KEY_A 0x04  // Keyboard a and A
#define KEY_B 0x05  // Keyboard b and B
#define KEY_C 0x06  // Keyboard c and C
#define KEY_D 0x07  // Keyboard d and D
#define KEY_E 0x08  // Keyboard e and E
#define KEY_F 0x09  // Keyboard f and F
#define KEY_G 0x0a  // Keyboard g and G
#define KEY_H 0x0b  // Keyboard h and H
#define KEY_I 0x0c  // Keyboard i and I
#define KEY_J 0x0d  // Keyboard j and J
#define KEY_K 0x0e  // Keyboard k and K
#define KEY_L 0x0f  // Keyboard l and L
#define KEY_M 0x10  // Keyboard m and M
#define KEY_N 0x11  // Keyboard n and N
#define KEY_O 0x12  // Keyboard o and O
#define KEY_P 0x13  // Keyboard p and P
#define KEY_Q 0x14  // Keyboard q and Q
#define KEY_R 0x15  // Keyboard r and R
#define KEY_S 0x16  // Keyboard s and S
#define KEY_T 0x17  // Keyboard t and T
#define KEY_U 0x18  // Keyboard u and U
#define KEY_V 0x19  // Keyboard v and V
#define KEY_W 0x1a  // Keyboard w and W
#define KEY_X 0x1b  // Keyboard x and X
#define KEY_Y 0x1c  // Keyboard y and Y
#define KEY_Z 0x1d  // Keyboard z and Z

#define KEY_1 0x1e  // Keyboard 1 and !
#define KEY_2 0x1f  // Keyboard 2 and @
#define KEY_3 0x20  // Keyboard 3 and #
#define KEY_4 0x21  // Keyboard 4 and $
#define KEY_5 0x22  // Keyboard 5 and %
#define KEY_6 0x23  // Keyboard 6 and ^
#define KEY_7 0x24  // Keyboard 7 and &
#define KEY_8 0x25  // Keyboard 8 and *
#define KEY_9 0x26  // Keyboard 9 and (
#define KEY_0 0x27  // Keyboard 0 and )

#define KEY_ENTER      0x28  // Keyboard Return (ENTER)
#define KEY_ESC        0x29  // Keyboard ESCAPE
#define KEY_BACKSPACE  0x2a  // Keyboard DELETE (Backspace)
#define KEY_TAB        0x2b  // Keyboard Tab
#define KEY_SPACE      0x2c  // Keyboard Spacebar
#define KEY_MINUS      0x2d  // Keyboard - and _
#define KEY_EQUAL      0x2e  // Keyboard = and +
#define KEY_LEFTBRACE  0x2f  // Keyboard [ and {
#define KEY_RIGHTBRACE 0x30  // Keyboard ] and }
#define KEY_BACKSLASH  0x31  // Keyboard \ and |
#define KEY_HASHTILDE  0x32  // Keyboard Non-US # and ~
#define KEY_SEMICOLON  0x33  // Keyboard ; and :
#define KEY_APOSTROPHE 0x34  // Keyboard ' and "
#define KEY_GRAVE      0x35  // Keyboard ` and ~
#define KEY_COMMA      0x36  // Keyboard , and <
#define KEY_DOT        0x37  // Keyboard . and >
#define KEY_SLASH      0x38  // Keyboard / and ?
#define KEY_CAPSLOCK   0x39  // Keyboard Caps Lock

#define KEY_F1  0x3a  // Keyboard F1
#define KEY_F2  0x3b  // Keyboard F2
#define KEY_F3  0x3c  // Keyboard F3
#define KEY_F4  0x3d  // Keyboard F4
#define KEY_F5  0x3e  // Keyboard F5
#define KEY_F6  0x3f  // Keyboard F6
#define KEY_F7  0x40  // Keyboard F7
#define KEY_F8  0x41  // Keyboard F8
#define KEY_F9  0x42  // Keyboard F9
#define KEY_F10 0x43  // Keyboard F10
#define KEY_F11 0x44  // Keyboard F11
#define KEY_F12 0x45  // Keyboard F12

#define KEY_PRTSC      0x46  // Keyboard Print Screen
#define KEY_SCROLLLOCK 0x47  // Keyboard Scroll Lock
#define KEY_PAUSE      0x48  // Keyboard Pause
#define KEY_INSERT     0x49  // Keyboard Insert
#define KEY_HOME       0x4a  // Keyboard Home
#define KEY_PAGEUP     0x4b  // Keyboard Page Up
// #define KEY_DELETE 0x4c // Keyboard Delete Forward
#define KEY_DELETE   0xD4  // Keyboard Delete Forward
#define KEY_END      0x4d  // Keyboard End
#define KEY_PAGEDOWN 0x4e  // Keyboard Page Down
#define KEY_RIGHT    0x4f  // Keyboard Right Arrow
#define KEY_LEFT     0x50  // Keyboard Left Arrow
#define KEY_DOWN     0x51  // Keyboard Down Arrow
#define KEY_UP       0x52  // Keyboard Up Arrow

#define KEY_NUMLOCK    0x53  // Keyboard Num Lock and Clear
#define KEY_KPSLASH    0x54  // Keypad /
#define KEY_KPASTERISK 0x55  // Keypad *
#define KEY_KPMINUS    0x56  // Keypad -
#define KEY_KPPLUS     0x57  // Keypad +
#define KEY_KPENTER    0x58  // Keypad ENTER
#define KEY_KP1        0x59  // Keypad 1 and End
#define KEY_KP2        0x5a  // Keypad 2 and Down Arrow
#define KEY_KP3        0x5b  // Keypad 3 and PageDn
#define KEY_KP4        0x5c  // Keypad 4 and Left Arrow
#define KEY_KP5        0x5d  // Keypad 5
#define KEY_KP6        0x5e  // Keypad 6 and Right Arrow
#define KEY_KP7        0x5f  // Keypad 7 and Home
#define KEY_KP8        0x60  // Keypad 8 and Up Arrow
#define KEY_KP9        0x61  // Keypad 9 and Page Up
#define KEY_KP0        0x62  // Keypad 0 and Insert
#define KEY_KPDOT      0x63  // Keypad . and Delete

#define KEY_102ND   0x64  // Keyboard Non-US \ and |
#define KEY_COMPOSE 0x65  // Keyboard Application
#define KEY_POWER   0x66  // Keyboard Power
#define KEY_KPEQUAL 0x67  // Keypad =

#define KEY_F13 0x68  // Keyboard F13
#define KEY_F14 0x69  // Keyboard F14
#define KEY_F15 0x6a  // Keyboard F15
#define KEY_F16 0x6b  // Keyboard F16
#define KEY_F17 0x6c  // Keyboard F17
#define KEY_F18 0x6d  // Keyboard F18
#define KEY_F19 0x6e  // Keyboard F19
#define KEY_F20 0x6f  // Keyboard F20
#define KEY_F21 0x70  // Keyboard F21
#define KEY_F22 0x71  // Keyboard F22
#define KEY_F23 0x72  // Keyboard F23
#define KEY_F24 0x73  // Keyboard F24

#define KEY_OPEN       0x74  // Keyboard Execute
#define KEY_HELP       0x75  // Keyboard Help
#define KEY_PROPS      0x76  // Keyboard Menu
#define KEY_FRONT      0x77  // Keyboard Select
#define KEY_STOP       0x78  // Keyboard Stop
#define KEY_AGAIN      0x79  // Keyboard Again
#define KEY_UNDO       0x7a  // Keyboard Undo
#define KEY_CUT        0x7b  // Keyboard Cut
#define KEY_COPY       0x7c  // Keyboard Copy
#define KEY_PASTE      0x7d  // Keyboard Paste
#define KEY_FIND       0x7e  // Keyboard Find
#define KEY_MUTE       0x7f  // Keyboard Mute
#define KEY_VOLUMEUP   0x80  // Keyboard Volume Up
#define KEY_VOLUMEDOWN 0x81  // Keyboard Volume Down
// 0x82  Keyboard Locking Caps Lock
// 0x83  Keyboard Locking Num Lock
// 0x84  Keyboard Locking Scroll Lock
#define KEY_KPCOMMA 0x85  // Keypad Comma
// 0x86  Keypad Equal Sign
#define KEY_RO               0x87  // Keyboard International1
#define KEY_KATAKANAHIRAGANA 0x88  // Keyboard International2
#define KEY_YEN              0x89  // Keyboard International3
#define KEY_HENKAN           0x8a  // Keyboard International4
#define KEY_MUHENKAN         0x8b  // Keyboard International5
#define KEY_KPJPCOMMA        0x8c  // Keyboard International6
// 0x8d  Keyboard International7
// 0x8e  Keyboard International8
// 0x8f  Keyboard International9
#define KEY_HANGEUL        0x90  // Keyboard LANG1
#define KEY_HANJA          0x91  // Keyboard LANG2
#define KEY_KATAKANA       0x92  // Keyboard LANG3
#define KEY_HIRAGANA       0x93  // Keyboard LANG4
#define KEY_ZENKAKUHANKAKU 0x94  // Keyboard LANG5
// 0x95  Keyboard LANG6
// 0x96  Keyboard LANG7
// 0x97  Keyboard LANG8
// 0x98  Keyboard LANG9
// 0x99  Keyboard Alternate Erase
// 0x9a  Keyboard SysReq/Attention
// 0x9b  Keyboard Cancel
// 0x9c  Keyboard Clear
// 0x9d  Keyboard Prior
// 0x9e  Keyboard Return
// 0x9f  Keyboard Separator
// 0xa0  Keyboard Out
// 0xa1  Keyboard Oper
// 0xa2  Keyboard Clear/Again
// 0xa3  Keyboard CrSel/Props
// 0xa4  Keyboard ExSel

// 0xb0  Keypad 00
// 0xb1  Keypad 000
// 0xb2  Thousands Separator
// 0xb3  Decimal Separator
// 0xb4  Currency Unit
// 0xb5  Currency Sub-unit
#define KEY_KPLEFTPAREN  0xb6  // Keypad (
#define KEY_KPRIGHTPAREN 0xb7  // Keypad )
// 0xb8  Keypad {
// 0xb9  Keypad }
// 0xba  Keypad Tab
#define KEY_KPBACKSPACE 0xbb  // Keypad Backspace
// 0xbc  Keypad A
// 0xbd  Keypad B
// 0xbe  Keypad C
// 0xbf  Keypad D
// 0xc0  Keypad E
// 0xc1  Keypad F
// 0xc2  Keypad XOR
// 0xc3  Keypad ^
// 0xc4  Keypad %
// 0xc5  Keypad <
// 0xc6  Keypad >
// 0xc7  Keypad &
// 0xc8  Keypad &&
// 0xc9  Keypad |
// 0xca  Keypad ||
// 0xcb  Keypad :
// 0xcc  Keypad #
// 0xcd  Keypad Space
// 0xce  Keypad @
// 0xcf  Keypad !
// 0xd0  Keypad Memory Store
// 0xd1  Keypad Memory Recall
// 0xd2  Keypad Memory Clear
// 0xd3  Keypad Memory Add
// 0xd4  Keypad Memory Subtract
// 0xd5  Keypad Memory Multiply
// 0xd6  Keypad Memory Divide
// 0xd7  Keypad +/-
// 0xd8  Keypad Clear
// 0xd9  Keypad Clear Entry
// 0xda  Keypad Binary
// 0xdb  Keypad Octal
// 0xdc  Keypad Decimal
// 0xdd  Keypad Hexadecimal

#define KEY_LEFTCTRL   0xe0  // Keyboard Left Control
#define KEY_LEFTSHIFT  0xe1  // Keyboard Left Shift
#define KEY_LEFTALT    0xe2  // Keyboard Left Alt
#define KEY_LEFTMETA   0xe3  // Keyboard Left GUI
#define KEY_RIGHTCTRL  0xe4  // Keyboard Right Control
#define KEY_RIGHTSHIFT 0xe5  // Keyboard Right Shift
#define KEY_RIGHTALT   0xe6  // Keyboard Right Alt
#define KEY_RIGHTMETA  0xe7  // Keyboard Right GUI

#define KEY_MEDIA_PLAYPAUSE    0xe8
#define KEY_MEDIA_STOPCD       0xe9
#define KEY_MEDIA_PREVIOUSSONG 0xea
#define KEY_MEDIA_NEXTSONG     0xeb
#define KEY_MEDIA_EJECTCD      0xec
#define KEY_MEDIA_VOLUMEUP     0xed
#define KEY_MEDIA_VOLUMEDOWN   0xee
#define KEY_MEDIA_MUTE         0xef
#define KEY_MEDIA_WWW          0xf0
#define KEY_MEDIA_BACK         0xf1
#define KEY_MEDIA_FORWARD      0xf2
#define KEY_MEDIA_STOP         0xf3
#define KEY_MEDIA_FIND         0xf4
#define KEY_MEDIA_SCROLLUP     0xf5
#define KEY_MEDIA_SCROLLDOWN   0xf6
#define KEY_MEDIA_EDIT         0xf7
#define KEY_MEDIA_SLEEP        0xf8
#define KEY_MEDIA_COFFEE       0xf9
#define KEY_MEDIA_REFRESH      0xfa
#define KEY_MEDIA_CALC         0xfb

enum {
    // Generic Control
    KEY_CONSUMER_CONTROL = 0x0001,

    // Power Control
    KEY_CONSUMER_POWER = 0x0030,
    KEY_CONSUMER_RESET = 0x0031,
    KEY_CONSUMER_SLEEP = 0x0032,

    // Screen Brightness
    KEY_CONSUMER_BRIGHTNESS_INCREMENT = 0x006F,
    KEY_CONSUMER_BRIGHTNESS_DECREMENT = 0x0070,

    // These HID usages operate only on mobile systems (battery powered) and
    // require Windows 8 (build 8302 or greater).
    KEY_CONSUMER_WIRELESS_RADIO_CONTROLS      = 0x000C,
    KEY_CONSUMER_WIRELESS_RADIO_BUTTONS       = 0x00C6,
    KEY_CONSUMER_WIRELESS_RADIO_LED           = 0x00C7,
    KEY_CONSUMER_WIRELESS_RADIO_SLIDER_SWITCH = 0x00C8,

    // Media Control
    KEY_CONSUMER_PLAY_PAUSE       = 0x00CD,
    KEY_CONSUMER_SCAN_NEXT        = 0x00B5,
    KEY_CONSUMER_SCAN_PREVIOUS    = 0x00B6,
    KEY_CONSUMER_STOP             = 0x00B7,
    KEY_CONSUMER_VOLUME           = 0x00E0,
    KEY_CONSUMER_MUTE             = 0x00E2,
    KEY_CONSUMER_BASS             = 0x00E3,
    KEY_CONSUMER_TREBLE           = 0x00E4,
    KEY_CONSUMER_BASS_BOOST       = 0x00E5,
    KEY_CONSUMER_VOLUME_INCREMENT = 0x00E9,
    KEY_CONSUMER_VOLUME_DECREMENT = 0x00EA,
    KEY_CONSUMER_BASS_INCREMENT   = 0x0152,
    KEY_CONSUMER_BASS_DECREMENT   = 0x0153,
    KEY_CONSUMER_TREBLE_INCREMENT = 0x0154,
    KEY_CONSUMER_TREBLE_DECREMENT = 0x0155,

    // Application Launcher
    KEY_CONSUMER_AL_CONSUMER_CONTROL_CONFIGURATION = 0x0183,
    KEY_CONSUMER_AL_EMAIL_READER                   = 0x018A,
    KEY_CONSUMER_AL_CALCULATOR                     = 0x0192,
    KEY_CONSUMER_AL_LOCAL_BROWSER                  = 0x0194,

    // Browser/Explorer Specific
    KEY_CONSUMER_AC_SEARCH    = 0x0221,
    KEY_CONSUMER_AC_HOME      = 0x0223,
    KEY_CONSUMER_AC_BACK      = 0x0224,
    KEY_CONSUMER_AC_FORWARD   = 0x0225,
    KEY_CONSUMER_AC_STOP      = 0x0226,
    KEY_CONSUMER_AC_REFRESH   = 0x0227,
    KEY_CONSUMER_AC_BOOKMARKS = 0x022A,

    // Mouse Horizontal scroll
    KEY_CONSUMER_AC_PAN = 0x0238,
};

/**
 * keytable structure
 */
typedef struct {
    uint8_t scan1, scan2;  // scan code
    uint8_t ascii;         // ASCII code
    uint8_t hidcode;       // HID code for keyboard page
} keytable_t;

/**
 * FN functions
 */
typedef enum {
    FN_NOP = 0,
    FN_FNLOCK,
    FN_BACKLIGHT,
} fn_function_t;

/**
 * FN keytable structure
 */
typedef struct {
    uint8_t scan1, scan2;  // scan code
    uint16_t hidcode;      // HID code for consumer page
    fn_function_t fncode;  // fn function type
} fn_keytable_t;

/**
 * search the USB HID key based on scan code
 * @param scan1 scan code 1
 * @param scan2 scan code 2
 * @return -1 if not found
 */
int search_hid_key(unsigned scan1, unsigned scan2);

/**
 * search the FN function based on scan code
 * @param scan1 scan code 1
 * @param scan2 scan code 2
 * @return NULL if not found
 */
fn_keytable_t* search_fn(unsigned scan1, unsigned scan2);

#endif
