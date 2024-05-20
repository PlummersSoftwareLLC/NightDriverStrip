/**
 * @file      LilyGo_Button.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2023-10-21
 *
 */

#pragma once


#include <Arduino.h>


#define DEBOUNCE_MS         (50)
#define LONGCLICK_MS        (250)
#define DOUBLECLICK_MS      (400)
#define LONGPRESS_MS        (1200)
#define SINGLE_CLICK        (1)
#define DOUBLE_CLICK        (2)
#define TRIPLE_CLICK        (3)
#define LONG_PRESS          (4)

enum ButtonState {
    BTN_PRESSED_EVENT,
    BTN_RELEASED_EVENT,
    BTN_CLICK_EVENT,
    BTN_LONG_PRESSED_EVENT,
    BTN_DOUBLE_CLICK_EVENT,
    BTN_TRIPLE_CLICK_EVENT,
};

class LilyGo_Button
{
    typedef void (*event_callback) (ButtonState state);
public:

    void init(uint32_t gpio, uint32_t debounceTimeout = DEBOUNCE_MS);
    void setDebounceTime(uint32_t ms);
    void setEventCallback(event_callback f);
    void update();
    uint32_t wasPressedFor();
    uint32_t getNumberOfClicks();
    uint32_t getClickType();
private:
    uint32_t gpio;
    int prev_state;
    int curr_state = HIGH;
    uint8_t click_count = 0;
    uint32_t last_click_type = 0;
    uint64_t click_ms;
    uint64_t down_ms;
    uint64_t long_down_ms = 0;
    uint32_t debounce_time_ms = DOUBLECLICK_MS;
    uint32_t down_time_ms = 0;
    bool pressed_triggered = false;
    bool longclick_detected = false;
    bool long_pressed_detected = false;
    event_callback event_cb = NULL;
};
