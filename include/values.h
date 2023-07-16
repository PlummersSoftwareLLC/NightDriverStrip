#include "types.h"

struct Values
{
    AppTime AppTime;                                                        // Keeps track of frame times
    volatile double FreeDrawTime;
    float Brite;
    uint32_t Watts;
    uint32_t FPS = 0;                                                       // Our global framerate
    bool g_bUpdateStarted = false;                                          // Has an OTA update started?
};