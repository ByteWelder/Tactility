#pragma once
#include <Tactility/hal/Configuration.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>

// Supported hardware:
#if defined(CONFIG_TT_BOARD_LILYGO_TDECK)
#include "LilygoTdeck.h"
#define TT_BOARD_HARDWARE &lilygo_tdeck
#elif defined(CONFIG_TT_BOARD_LILYGO_TLORA_PAGER)
#include "LilygoTloraPager.h"
#define TT_BOARD_HARDWARE &lilygo_tlora_pager
#elif defined(CONFIG_TT_BOARD_CYD_2432S024C)
#include "CYD2432S024C.h"
#define TT_BOARD_HARDWARE &cyd_2432s024c_config
#elif defined(CONFIG_TT_BOARD_CYD_2432S028R)
#include "CYD2432S028R.h"
#define TT_BOARD_HARDWARE &cyd_2432s028r_config
#elif defined(CONFIG_TT_BOARD_CYD_E32R28T)
#include "E32R28T.h"
#define TT_BOARD_HARDWARE &cyd_e32r28t_config
#elif defined(CONFIG_TT_BOARD_CYD_2432S032C)
#include "CYD2432S032C.h"
#define TT_BOARD_HARDWARE &cyd_2432S032c_config
#elif (defined(CONFIG_TT_BOARD_ELECROW_CROWPANEL_ADVANCE_28))
#define TT_BOARD_HARDWARE &crowpanel_advance_28
#include "CrowPanelAdvance28.h"
#elif (defined(CONFIG_TT_BOARD_ELECROW_CROWPANEL_ADVANCE_35))
#define TT_BOARD_HARDWARE &crowpanel_advance_35
#include "CrowPanelAdvance35.h"
#elif (defined(CONFIG_TT_BOARD_ELECROW_CROWPANEL_ADVANCE_50))
#define TT_BOARD_HARDWARE &crowpanel_advance_50
#include "CrowPanelAdvance50.h"
#elif (defined(CONFIG_TT_BOARD_ELECROW_CROWPANEL_BASIC_28))
#define TT_BOARD_HARDWARE &crowpanel_basic_28
#include "CrowPanelBasic28.h"
#elif (defined(CONFIG_TT_BOARD_ELECROW_CROWPANEL_BASIC_35))
#define TT_BOARD_HARDWARE &crowpanel_basic_35
#include "CrowPanelBasic35.h"
#elif (defined(CONFIG_TT_BOARD_ELECROW_CROWPANEL_BASIC_50))
#define TT_BOARD_HARDWARE &crowpanel_basic_50
#include "CrowPanelBasic50.h"
#elif defined(CONFIG_TT_BOARD_M5STACK_CARDPUTER)
#include "M5stackCardputer.h"
#define TT_BOARD_HARDWARE &m5stack_cardputer
#elif defined(CONFIG_TT_BOARD_M5STACK_CORE2)
#include "M5stackCore2.h"
#define TT_BOARD_HARDWARE &m5stack_core2
#elif defined(CONFIG_TT_BOARD_M5STACK_CORES3)
#include "M5stackCoreS3.h"
#define TT_BOARD_HARDWARE &m5stack_cores3
#elif defined(CONFIG_TT_BOARD_UNPHONE)
#include "UnPhone.h"
#define TT_BOARD_HARDWARE &unPhone
#elif defined(CONFIG_TT_BOARD_CYD_JC2432W328C)
#include "JC2432W328C.h"
#define TT_BOARD_HARDWARE &cyd_jc2432w328c_config
#elif defined(CONFIG_TT_BOARD_CYD_8048S043C)
#include "CYD8048S043C.h"
#define TT_BOARD_HARDWARE &cyd_8048s043c_config
#elif defined(CONFIG_TT_BOARD_CYD_JC8048W550C)
#include "JC8048W550C.h"
#define TT_BOARD_HARDWARE &cyd_jc8048w550c_config
#elif defined(CONFIG_TT_BOARD_CYD_4848S040C)
#include "CYD4848S040C.h"
#define TT_BOARD_HARDWARE &cyd_4848s040c_config
#elif defined(CONFIG_TT_BOARD_WAVESHARE_S3_TOUCH_43)
#include "WaveshareS3Touch43.h"
#define TT_BOARD_HARDWARE &waveshare_s3_touch_43
#elif defined(CONFIG_TT_BOARD_WAVESHARE_S3_TOUCH_LCD_147)
#include "WaveshareS3TouchLcd147.h"
#define TT_BOARD_HARDWARE &waveshare_s3_touch_lcd_147
#elif defined(CONFIG_TT_BOARD_WAVESHARE_S3_TOUCH_LCD_128)
#include "WaveshareS3TouchLcd128.h"
#define TT_BOARD_HARDWARE &waveshare_s3_touch_lcd_128
#elif defined(CONFIG_TT_BOARD_WAVESHARE_S3_LCD_13)
#include "WaveshareS3Lcd13.h"
#define TT_BOARD_HARDWARE &waveshare_s3_lcd_13
#else
#define TT_BOARD_HARDWARE NULL
#error Replace TT_BOARD_HARDWARE in main.c with your own. Or copy one of the ./sdkconfig.board.* files into ./sdkconfig.
#endif

#else // else simulator

#include "Simulator.h"

extern tt::hal::Configuration hardware;
#define TT_BOARD_HARDWARE &hardware

#endif // ESP_PLATFORM
