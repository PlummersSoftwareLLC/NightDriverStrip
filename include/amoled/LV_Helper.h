#pragma once

/**
 * @file      LV_Helper.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-04-20
 *
 */

#include "globals.h"

#include "LilyGo_Display.h"
#include <lvgl.h>


void beginLvglHelper(LilyGo_Display &board, bool debug = false);
