#pragma once

// Display backlight (PWM)
#define CYD4848S040_LCD_BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define CYD4848S040_LCD_BACKLIGHT_LEDC_MODE LEDC_LOW_SPEED_MODE
#define CYD4848S040_LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_0
#define CYD4848S040_LCD_BACKLIGHT_LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define CYD4848S040_LCD_BACKLIGHT_LEDC_FREQUENCY (1000)

#define CYD4848S040_LCD_PIN_BACKLIGHT GPIO_NUM_38

// Display pins
#define CYD4848S040_LCD_PIN_HSYNC   GPIO_NUM_16 // HSYNC
#define CYD4848S040_LCD_PIN_VSYNC   GPIO_NUM_17 // VSYNC
#define CYD4848S040_LCD_PIN_DE      GPIO_NUM_18 // DE
#define CYD4848S040_LCD_PIN_PCLK    GPIO_NUM_21 // PCLK

#define CYD4848S040_LCD_PIN_DATA0   GPIO_NUM_4  // B1
#define CYD4848S040_LCD_PIN_DATA1   GPIO_NUM_5  // B2
#define CYD4848S040_LCD_PIN_DATA2   GPIO_NUM_6  // B3
#define CYD4848S040_LCD_PIN_DATA3   GPIO_NUM_7  // B4
#define CYD4848S040_LCD_PIN_DATA4   GPIO_NUM_15 // B5

#define CYD4848S040_LCD_PIN_DATA5   GPIO_NUM_8  // G1
#define CYD4848S040_LCD_PIN_DATA6   GPIO_NUM_20 // G2
#define CYD4848S040_LCD_PIN_DATA7   GPIO_NUM_3  // G3
#define CYD4848S040_LCD_PIN_DATA8   GPIO_NUM_46 // G4
#define CYD4848S040_LCD_PIN_DATA9   GPIO_NUM_9  // G5
#define CYD4848S040_LCD_PIN_DATA10  GPIO_NUM_10 // G6

#define CYD4848S040_LCD_PIN_DATA11  GPIO_NUM_11 // R1
#define CYD4848S040_LCD_PIN_DATA12  GPIO_NUM_12 // R2
#define CYD4848S040_LCD_PIN_DATA13  GPIO_NUM_13 // R3
#define CYD4848S040_LCD_PIN_DATA14  GPIO_NUM_14 // R4
#define CYD4848S040_LCD_PIN_DATA15  GPIO_NUM_0  // R5

#define CYD4848S040_LCD_PIN_CS      GPIO_NUM_39 //CS
#define CYD4848S040_LCD_PIN_DISP_EN GPIO_NUM_NC // not connected

// Display
#define CYD4848S040_LCD_HORIZONTAL_RESOLUTION 480
#define CYD4848S040_LCD_VERTICAL_RESOLUTION 480
#define CYD4848S040_LCD_DRAW_BUFFER_SIZE (CYD4848S040_LCD_HORIZONTAL_RESOLUTION * CYD4848S040_LCD_VERTICAL_RESOLUTION)
