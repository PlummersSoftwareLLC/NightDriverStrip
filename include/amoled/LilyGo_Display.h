/**
 * @file      LilyGo_Display.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-06
 *
 */
#pragma once

#include <stdint.h>

// enum DispRotation {
//     DISP_VERTICAL,      // vertical
//     DISP_HORIZONTAL,    // horizontal
// };

class LilyGo_Display
{
public:
    LilyGo_Display(): _rotation(0) {};
    virtual void setRotation(uint8_t rotation) = 0;
    virtual uint8_t getRotation() = 0;
    virtual void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) = 0;
    virtual void pushColors(uint16_t *data, uint32_t len) = 0;
    virtual void pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data) = 0;
    virtual uint16_t  width() = 0;
    virtual uint16_t  height() = 0;

    virtual uint8_t getPoint(int16_t *x, int16_t *y, uint8_t get_point ) = 0;
    virtual bool    hasTouch() = 0;

protected:
    uint8_t _rotation;
};
