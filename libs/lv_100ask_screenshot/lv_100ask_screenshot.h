/**
 * @file lv_100ask_screenshot.h
 *
 */

#pragma once

#include <stdbool.h>
#include "lvgl.h"


#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
	LV_100ASK_SCREENSHOT_SV_BMP  = 0,
	LV_100ASK_SCREENSHOT_SV_PNG  = 1,
	LV_100ASK_SCREENSHOT_SV_LAST
}lv_100ask_screenshot_sv_t;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
bool lv_100ask_screenshot_create(lv_obj_t * obj,  lv_img_cf_t cf, lv_100ask_screenshot_sv_t screenshot_sv, const char * filename);

/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif
