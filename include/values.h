#pragma once

#include <esp_attr.h>
#include "globals.h"
#include "types.h"

struct Values
{
    CAppTime AppTime;                                                       // Keeps track of frame times
    volatile double FreeDrawTime = 0.0;
    float Brite;
    uint32_t Watts;
    uint32_t FPS = 0;                                                       // Our global framerate
    bool UpdateStarted = false;                                             // Has an OTA update started?
    uint8_t Brightness = 255;
    uint8_t Fader = 255;
#if USE_MATRIX
    int MatrixPowerMilliwatts = 0;                                         // Matrix power draw in mw
    uint8_t MatrixScaledBrightness = 255;                                  // 0-255 scaled brightness to stay in limit
#endif
};

extern DRAM_ATTR Values g_Values;