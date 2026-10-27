/**
 * Host-simulator LVGL config. Included before the defaults in lv_conf_internal.h,
 * so anything left unset keeps the LVGL 8.3 default.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* SDL wants 32-bit pixels. The device panel is RGB565; indexed icons still decode. */
#define LV_COLOR_DEPTH 32

#define LV_MEM_SIZE (256U * 1024U)

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* Fonts the device UI actually references. Montserrat 18 stays off, matching the device. */
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1

#define LV_USE_GRIDNAV 1

#endif
