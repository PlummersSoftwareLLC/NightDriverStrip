#pragma once

#include <esp_attr.h>
#include "types.h"

struct Values
{
    CAppTime AppTime;                                                        // Keeps track of frame times
    volatile double FreeDrawTime = 0.0;
    float Brite;
    uint32_t Watts;
    uint32_t FPS = 0;                                                       // Our global framerate
    bool UpdateStarted = false;                                             // Has an OTA update started?
    uint8_t Brightness = 255;
    uint8_t Fader = 255;
};

extern DRAM_ATTR Values g_Values;