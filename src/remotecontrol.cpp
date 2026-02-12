//+--------------------------------------------------------------------------
//
// File:        remotecontrol.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//
// Description:
//
//    Handles a simple IR remote for changing effects, brightness, etc.
//    Native ESP32 RMT implementation to eliminate the 4GB build churn
//    of the IRremoteESP8266 library.
//
// History:     Jun-14-2023     Rbergen                      Extracted handle() from header
//              Feb-12-2026     Antigravity & Robert Lipe    Replaced library with native RMT decoder
//---------------------------------------------------------------------------

#include "globals.h"

#if ENABLE_REMOTE

#include <driver/rmt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

#include "remotecontrol.h"
#include "systemcontainer.h"
#include "effects/strip/misceffects.h"

// ---------------------------------------------------------
// IR Key Definitions (NEC Protocol)
// ---------------------------------------------------------

// Set to 1 to enable the 44-Key Remote (White remote with DIY keys)
// Set to 0 to enable the 24-Key Remote (Black/Color remote)
#define REMOTE_KEY44 0

#if !REMOTE_KEY44
// 24-Key Remote Definitions
#define IR_ON      0xF7C03F
#define IR_OFF     0xF740BF
#define IR_BPLUS   0xF700FF
#define IR_BMINUS  0xF7807F
#define IR_R       0xF720DF
#define IR_G       0xF7A05F
#define IR_B       0xF7609F
#define IR_W       0xF7E01F
#define IR_B1      0xF710EF
#define IR_B2      0xF7906F
#define IR_B3      0xF750AF
#define IR_B4      0xF730CF
#define IR_B5      0xF7B04F
#define IR_B6      0xF7708F
#define IR_B7      0xF708F7
#define IR_B8      0xF78877
#define IR_B9      0xF748B7
#define IR_B10     0xF728D7
#define IR_B11     0xF7A857
#define IR_B12     0xF76897
#define IR_FLASH   0xF7D02F
#define IR_STROBE  0xF7F00F
#define IR_FADE    0xF7C837
#define IR_SMOOTH  0xF7E817

static const RemoteColorCode RemoteColorCodes[] = 
{
    { IR_OFF, CRGB(0, 0, 0),       0,   "Off"          },
    { IR_R,   CRGB(255, 0, 0),     0,   "Red"          },
    { IR_G,   CRGB(0, 255, 0),     96,  "Green"        },
    { IR_B,   CRGB(0, 0, 255),     160, "Blue"         },
    { IR_W,   CRGB(255, 255, 255), 0,   "White"        },
    { IR_B1,  CRGB(255, 64, 0),    16,  "Red-Orange"   },
    { IR_B2,  CRGB(0, 255, 64),    112, "Light Green"  },
    { IR_B3,  CRGB(64, 0, 255),    176, "Indigo"       },
    { IR_B4,  CRGB(255, 128, 0),   32,  "Orange"       },
    { IR_B5,  CRGB(0, 255, 128),   128, "Cyan-Green"   },
    { IR_B6,  CRGB(128, 0, 255),   192, "Purple"       },
    { IR_B7,  CRGB(255, 192, 0),   48,  "Yellow-Orange"},
    { IR_B8,  CRGB(0, 255, 192),   112, "Cyan"         },
    { IR_B9,  CRGB(192, 0, 255),   208, "Magenta"      },
    { IR_B10, CRGB(255, 255, 0),   64,  "Yellow"       },
    { IR_B11, CRGB(0, 255, 255),   144, "Sky Blue"     },
    { IR_B12, CRGB(255, 0, 255),   224, "Pink"         }
};

#else
// 44-Key Remote Definitions
#define IR_BPLUS  0xFF3AC5  // Brightness Up
#define IR_BMINUS 0xFFBA45  // Brightness Down
#define IR_OFF    0xFF02FD  // Play/Pause (used as Off/On toggle in some logic)
#define IR_ON     0xFF827D  // On/Off (Power)

#define IR_R      0xFF1AE5  // Red
#define IR_G      0xFF9A65  // Green
#define IR_B      0xFFA25D  // Blue
#define IR_W      0xFF22DD  // White

#define IR_B1     0xFF2AD5  // Red-Orange
#define IR_B2     0xFFAA55  // Green-Light
#define IR_B3     0xFF926D  // Blue-Light
#define IR_B4     0xFF12ED  // Peach?

#define IR_B5     0xFF0AF5  // Orange
#define IR_B6     0xFF8A75  // Cyan
#define IR_B7     0xFFB24D  // Purple
#define IR_B8     0xFF32CD  // Pink?

#define IR_B9     0xFF38C7  // Yellow-Orange
#define IR_B10    0xFFB847  // Light Blue
#define IR_B11    0xFF7887  // Magenta
#define IR_B12    0xFFF807  // Light Yellow

#define IR_B13    0xFF18E7  // Sky Blue
#define IR_B14    0xFF9867  // Pink
#define IR_B15    0xFF58A7  // Aqua
#define IR_B16    0xFFD827  // Blue-White

#define IR_UPR    0xFF28D7  // Red Up
#define IR_UPG    0xFFA857  // Green Up
#define IR_UPB    0xFF6897  // Blue Up
#define IR_QUICK  0xFFE817  // Quick

#define IR_DOWNR  0xFF08F7  // Red Down
#define IR_DOWNG  0xFF8877  // Green Down
#define IR_DOWNB  0xFF48B7  // Blue Down
#define IR_SLOW   0xFFC837  // Slow

#define IR_DIY1   0xFF30CF  // DIY1
#define IR_DIY2   0xFFB04F  // DIY2
#define IR_DIY3   0xFF708F  // DIY3
#define IR_AUTO   0xFFF00F  // Auto

#define IR_DIY4   0xFF10EF  // DIY4
#define IR_DIY5   0xFF906F  // DIY5
#define IR_DIY6   0xFF50AF  // DIY6
#define IR_FLASH  0xFFD02F  // Flash

#define IR_JUMP3  0xFF20DF  // Jump3
#define IR_JUMP7  0xFFA05F  // Jump7
#define IR_FADE3  0xFF609F  // Fade3 / Fade
#define IR_FADE7  0xFFE01F  // Fade7

// Map required keys for logic compatibility
#define IR_FADE   IR_FADE3 
#define IR_SMOOTH IR_AUTO   // Map Smooth to Auto

static const RemoteColorCode RemoteColorCodes[] = 
{
    { IR_OFF,   CRGB(0, 0, 0),       0,   "Off"          },
    { IR_R,     CRGB(255, 0, 0),     0,   "Red"          },
    { IR_G,     CRGB(0, 255, 0),     96,  "Green"        },
    { IR_B,     CRGB(0, 0, 255),     160, "Blue"         },
    { IR_W,     CRGB(255, 255, 255), 0,   "White"        },
    { IR_B1,    CRGB(255, 64, 0),    16,  "Red-Orange"   },
    { IR_B2,    CRGB(0, 255, 64),    112, "Light Green"  },
    { IR_B3,    CRGB(64, 0, 255),    176, "Indigo"       },
    { IR_B4,    CRGB(255, 128, 0),   32,  "Orange"       },
    { IR_B5,    CRGB(0, 255, 128),   128, "Cyan-Green"   },
    { IR_B6,    CRGB(0, 128, 255),   160, "Light Blue"   }, // Adjusted guess
    { IR_B7,    CRGB(128, 0, 255),   192, "Purple"       },
    { IR_B8,    CRGB(255, 0, 128),   224, "Pink"         },
    { IR_B9,    CRGB(255, 128, 0),   48,  "Yellow-Orange"}, // Repetitive
    { IR_B10,   CRGB(0, 255, 128),   112, "Cyan"         }, // Repetitive
    { IR_B11,   CRGB(128, 0, 255),   208, "Magenta"      },
    { IR_B12,   CRGB(255, 255, 0),   64,  "Yellow"       },
    // The 44 key remote has more "color" buttons (B13-B16) but our struct
    // and standard array usually stop at B12. We can add them if needed.
    // For now, mapping the core set.
    { IR_DIY1,  CRGB(255, 0, 0),     0,   "DIY1 (Red)"   },
    { IR_DIY2,  CRGB(0, 255, 0),     96,  "DIY2 (Green)" },
    { IR_DIY3,  CRGB(0, 0, 255),     160, "DIY3 (Blue)"  },
};
#endif

// ---------------------------------------------------------
// Native RMT Decoder Implementation (Legacy 4.x API)
// ---------------------------------------------------------

#define NEC_DECODE_MARGIN 200  // Tolerance in microseconds
#define RMT_RESOLUTION_HZ 1000000 

class RemoteControlImpl 
{
public:
    RemoteControlImpl(int pin) : _pin(pin), _channel(RMT_CHANNEL_0) {}
    
    ~RemoteControlImpl() {
        if (_begun) {
            rmt_rx_stop(_channel);
            rmt_driver_uninstall(_channel);
        }
    }

    bool begin() {
        rmt_config_t config = RMT_DEFAULT_CONFIG_RX((gpio_num_t)_pin, _channel);
        config.clk_div = 80; // 80MHz / 80 = 1MHz resolution (1us per tick)
        config.rx_config.filter_en = true;
        config.rx_config.filter_ticks_thresh = 100; // Ignore pulses shorter than 100us
        config.rx_config.idle_threshold = 20000;    // 20ms idle = end of frame

        if (rmt_config(&config) != ESP_OK) return false;
        if (rmt_driver_install(_channel, 1024, 0) != ESP_OK) return false;
        if (rmt_get_ringbuf_handle(_channel, &_ringbuf) != ESP_OK) return false;
        if (rmt_rx_start(_channel, true) != ESP_OK) return false;

        _begun = true;
        return true;
    }

    bool decode(uint32_t &code, bool &isRepeat) {
        if (!_begun) return false;

        size_t size = 0;
        rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(_ringbuf, &size, 0);
        if (items) {
            bool success = parseNecFrame(items, size / sizeof(rmt_item32_t), code, isRepeat);
            vRingbufferReturnItem(_ringbuf, items);
            return success;
        }
        return false;
    }

private:
    int _pin;
    rmt_channel_t _channel;
    RingbufHandle_t _ringbuf = NULL;
    bool _begun = false;

    bool match(uint32_t measured, uint32_t target) {
        return (measured >= (target - NEC_DECODE_MARGIN)) && 
               (measured <= (target + NEC_DECODE_MARGIN));
    }

    bool parseNecFrame(rmt_item32_t* items, size_t count, uint32_t &code, bool &isRepeat) {
        isRepeat = false; // Always reset first

        // Linearized access to durations
        auto get_time = [&](int i) -> uint32_t {
            if (i % 2 == 0) return items[i/2].duration0;
            else return items[i/2].duration1;
        };

        size_t symbol_count = count * 2;
        if (symbol_count < 2) return false;

        // Leader Code (9ms Mark, 4.5ms Space / 2.25ms Repeat)
        if (!match(get_time(0), 9000)) return false;
        
        if (match(get_time(1), 2250)) {
            isRepeat = true;
            return true;
        }
        if (!match(get_time(1), 4500)) return false;

        if (symbol_count < 67) return false;

        uint32_t data = 0;
        int bit_idx = 0;

        for (int i = 2; i < 66; i += 2) {
            if (!match(get_time(i), 560)) return false;
            
            data <<= 1;
            
            // 560us space = '0', 1690us space = '1'
            if (match(get_time(i+1), 1690)) {
                data |= 1;
            } else if (!match(get_time(i+1), 560)) {
                return false; 
            }
        }
        
        code = data;
        isRepeat = false;
        return true;
    }
};

// ---------------------------------------------------------
// RemoteControl Wrappers
// ---------------------------------------------------------

RemoteControl::RemoteControl() : _pImpl(std::make_unique<RemoteControlImpl>(IR_REMOTE_PIN)) {}
RemoteControl::~RemoteControl() = default;

bool RemoteControl::begin() {
    debugW("Native Remote Control Decoding Started (RMT Legacy)");
    return _pImpl->begin();
}

void RemoteControl::end() {
    debugW("Native Remote Control Decoding Stopped");
}

#define BRIGHTNESS_STEP     20

void RemoteControl::handle()
{
    uint32_t result = 0;
    bool isRepeat = false;
    static uint32_t lastResult = 0;

    if (!_pImpl->decode(result, isRepeat))
        return;

    if (isRepeat) {
        result = lastResult;
    }

    if (result == 0) return;

    // debugI("Received IR Remote Code: 0x%08lX %s\n", (unsigned long)result, isRepeat ? "(Repeat)" : "");

    if (isRepeat || result == lastResult)
    {
        static uint lastRepeatTime = millis();
        auto kMinRepeatms = 200;
        
        if (result == IR_OFF)
            kMinRepeatms = 0;               
        else if (isRepeat)
            kMinRepeatms = 500;             
        else if (result == lastResult)
            kMinRepeatms = 50;              

        if (millis() - lastRepeatTime <= kMinRepeatms)
            return;

        lastRepeatTime = millis();
    }
    
    lastResult = result;

    auto &effectManager = g_ptrSystem->EffectManager();
    auto &deviceConfig = g_ptrSystem->DeviceConfig();

    if (IR_ON == result)
    {
        debugI("Remote: Power ON");
        effectManager.ClearRemoteColor();
        effectManager.SetInterval(0);
        effectManager.StartEffect();
        deviceConfig.SetBrightness(BRIGHTNESS_MAX);
        return;
    }
    else if (IR_OFF == result)
    {
        debugI("Remote: Power OFF");
        #if USE_HUB75
            deviceConfig.SetBrightness((int)deviceConfig.GetBrightness() - BRIGHTNESS_STEP);
        #else
            effectManager.ClearRemoteColor();
            effectManager.SetInterval(0);
            effectManager.StartEffect();
            deviceConfig.SetBrightness(0);
        #endif
        return;
    }
    else if (IR_BPLUS == result)
    {
        debugI("Remote: Bright/Speed +");
        effectManager.SetInterval(DEFAULT_EFFECT_INTERVAL, true);
        if (deviceConfig.ApplyGlobalColors())
            effectManager.ClearRemoteColor();
        else
            effectManager.NextEffect();

        return;
    }
    else if (IR_BMINUS == result)
    {
        debugI("Remote: Bright/Speed -");
        effectManager.SetInterval(DEFAULT_EFFECT_INTERVAL, true);
        if (deviceConfig.ApplyGlobalColors())
            effectManager.ClearRemoteColor();
        else
            effectManager.PreviousEffect();

        return;
    }
    else if (IR_SMOOTH == result)
    {
        debugI("Remote: Smooth");
        effectManager.ClearRemoteColor();
        effectManager.SetInterval(EffectManager::csSmoothButtonSpeed);
    }
    else if (IR_STROBE == result)
    {
        debugI("Remote: Strobe");
        effectManager.NextPalette();
    }
    else if (IR_FLASH == result)
    {
        debugI("Remote: Flash");
        effectManager.PreviousPalette();
    }
    else if (IR_FADE == result)
    {
        debugI("Remote: Fade");
        effectManager.ShowVU( !effectManager.IsVUVisible() );
    }

    for (const auto & RemoteColorCode : RemoteColorCodes)
    {
        if (RemoteColorCode.code == result)
        {
            debugI("Remote: Color %s (0x%08lX)", RemoteColorCode.name, (unsigned long)(uint32_t) RemoteColorCode.color);
            effectManager.ApplyGlobalColor(RemoteColorCode.color);
            #if FULL_COLOR_REMOTE_FILL
                auto effect = make_shared_psram<ColorFillEffect>("Remote Color", RemoteColorCode.color, 1, true);
                if (effect->Init(g_ptrSystem->EffectManager().GetBaseGraphics()))
                    g_ptrSystem->EffectManager().SetTempEffect(effect);
                else
                    debugE("Could not initialize new color fill effect");
            #endif
            return;
        }
    }
    
    // Log unknown codes
    debugD("Remote: Unknown Code 0x%08lX", (unsigned long)result);
}

#endif
