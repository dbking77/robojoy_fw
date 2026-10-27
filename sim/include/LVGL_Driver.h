#pragma once

#include <stdio.h>

/* The UI includes this header but does not call the panel driver.
 * The host build supplies this stub so ESP-IDF headers stay out of the simulator.
 * stdio.h matches the device header, which the UI relies on for snprintf. */
#include "lvgl.h"
