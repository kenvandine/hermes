/**
 * LVGL Configuration for ESP32 Intercom
 *
 * Minimal configuration to enable required fonts and features
 * for the Waveshare ESP32-S3 1.8" AMOLED display
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// Color depth (16-bit RGB565 for AMOLED)
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

// Memory settings
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)  // 64KB for LVGL heap

// Display settings
#define LV_HOR_RES_MAX 368
#define LV_VER_RES_MAX 448
#define LV_DPI_DEF 130

// Enable fonts we need
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_48 1

// Disable fonts we don't need to save memory
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0

// Default font
#define LV_FONT_DEFAULT &lv_font_montserrat_18

// Enable built-in symbols (icons)
#define LV_USE_FONT_COMPRESSED 0
#define LV_FONT_SUBPX 0

// Widget settings
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       0
#define LV_USE_ROLLER     0
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      0

// Animation
#define LV_USE_ANIMATION 1
#define LV_ANIM_LIMIT 1024

// Logging (for debugging)
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

// Performance settings
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_REFR_DEBUG 0

// Themes
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_BASIC 1
#define LV_USE_THEME_MONO 0

// Tick
#define LV_TICK_CUSTOM 0

// Assert handler
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);

// Other settings
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_USE_USER_DATA 1

// Sprintf
#define LV_SPRINTF_CUSTOM 0
#define LV_SPRINTF_USE_FLOAT 0

// GPU (not available on ESP32)
#define LV_USE_GPU_SDL 0
#define LV_USE_GPU_STM32_DMA2D 0

#endif // LV_CONF_H
