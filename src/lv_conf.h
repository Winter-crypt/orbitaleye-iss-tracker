/**
 * lv_conf.h — OrbitalEye CYD
 * LVGL 8.3.x config for ESP32 + ILI9341 320x240
 */

#if 1  /* Set this to "1" to enable the content */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   1
#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (48U * 1024U)

/* HAL tick via Arduino millis() */
#define LV_TICK_CUSTOM                  1
#define LV_TICK_CUSTOM_INCLUDE          "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR    (millis())

#define LV_HOR_RES_MAX   320
#define LV_VER_RES_MAX   240

/* Fonts */
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_12

/* Logging */
#define LV_USE_LOG         1
#define LV_LOG_LEVEL       LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF      1

/* Widgets usati */
#define LV_USE_BTN         1
#define LV_USE_LABEL       1
#define LV_USE_ARC         1
#define LV_USE_LINE        1
#define LV_USE_CANVAS      1
#define LV_USE_IMG         1
#define LV_USE_ANIMATION   1

/* Widget non usati (disabilitati per risparmiare RAM) */
#define LV_USE_METER       0
#define LV_USE_TABVIEW     0
#define LV_USE_TILEVIEW    0
#define LV_USE_WIN         0
#define LV_USE_SPAN        0
#define LV_USE_MSGBOX      0
#define LV_USE_SPINBOX     0
#define LV_USE_SPINNER     0
#define LV_USE_KEYBOARD    0
#define LV_USE_CHART       0
#define LV_USE_TABLE       0
#define LV_USE_MENU        0
#define LV_USE_LED         0
#define LV_USE_DROPDOWN    1
#define LV_USE_ROLLER      0
#define LV_USE_SLIDER      0
#define LV_USE_SWITCH      0
#define LV_USE_TEXTAREA    0
#define LV_USE_CHECKBOX    0
#define LV_USE_IMGBTN      0
#define LV_USE_LIST        0
#define LV_USE_CALENDAR    0

/* Assert */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#endif /* LV_CONF_H */
#endif /* Enable content */
