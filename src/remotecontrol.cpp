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

#include <esp_idf_version.h>
#include <esp_intr_alloc.h>
#include <freertos/FreeRTOS.h>
#include <mutex>

// IR RX has two RMT backends, picked at compile time based on IDF version:
//
//   - The LEGACY ESP-IDF RMT API (driver/rmt.h) on IDF 4.x. This is the
//     only choice on Arduino-ESP32 2.x and is the path the project has
//     used for years. It exposes a 100us hardware glitch filter via
//     rx_config.filter_ticks_thresh, which is the sweet spot for cheap
//     IR remotes on noisy demodulators.
//
//   - The new driver_ng API (driver/rmt_rx.h) on IDF 5.x. Required on
//     Arduino-ESP32 3.x because the framework whole-archive-links
//     libesp_driver_rmt.a, and the legacy driver's conflict ctor in
//     rmt_legacy.c aborts boot when both are present. driver_ng's
//     hardware filter caps at ~3us so we lean on software-side
//     glitch filtering and wider parser tolerances to compensate.
//
// Each backend has its own RemoteControlImpl subclass below. They share
// the NEC parser machinery in the anonymous namespace.
//
// IMPORTANT: legacy and driver_ng headers cannot be included in the same
// translation unit on IDF 5.x - they both typedef the name rmt_channel_t,
// the legacy as an enum and the new as `struct rmt_channel_t *`, and the
// resulting "using typedef-name 'rmt_channel_t' after 'struct'" error is
// fatal. We only include the header for the backend we're actually using
// in this build.
#if ESP_IDF_VERSION_MAJOR >= 5
#include <atomic>
#include <driver/gpio.h>
#include <driver/rmt_rx.h>
#include <esp_heap_caps.h>
#include <freertos/queue.h>
#else
#include <driver/rmt.h>
#include <freertos/ringbuf.h>
#endif

#include "deviceconfig.h"
#include "effectmanager.h"
#include "gfxbase.h"
#include "ledstripeffect.h"
#include "remotecontrol.h"
#include "systemcontainer.h"
#include "taskmgr.h"   // REMOTE_STACK_SIZE / REMOTE_PRIORITY / REMOTE_CORE

#include "effects/strip/misceffects.h"

// RemoteColorCode
//
// Maps an IR remote code to a color and a name

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
    { IR_DIY4,  CRGB(255, 0, 0),     0,   "DIY4 (Red)"   },
    { IR_DIY5,  CRGB(0, 255, 0),     96,  "DIY5 (Green)" },
    { IR_DIY6,  CRGB(0, 0, 255),     160, "DIY6 (Blue)"  },
    { IR_JUMP3, CRGB(0, 0, 0),       0,   "Jump 3"       },
    { IR_JUMP7, CRGB(0, 0, 0),       0,   "Jump 7"       },
    { IR_FADE3, CRGB(0, 0, 0),       0,   "Fade 3"       },
    { IR_FADE7, CRGB(0, 0, 0),       0,   "Fade 7"       },
    { IR_QUICK, CRGB(0, 0, 0),       0,   "Quick"        },
    { IR_SLOW,  CRGB(0, 0, 0),       0,   "Slow"         },
    { IR_AUTO,  CRGB(0, 0, 0),       0,   "Auto"         },
    { IR_FLASH, CRGB(0, 0, 0),       0,   "Flash"        },
    { IR_UPR,   CRGB(0, 0, 0),       0,   "Red Up"       },
    { IR_UPG,   CRGB(0, 0, 0),       0,   "Green Up"     },
    { IR_UPB,   CRGB(0, 0, 0),       0,   "Blue Up"      },
    { IR_DOWNR, CRGB(0, 0, 0),       0,   "Red Down"     },
    { IR_DOWNG, CRGB(0, 0, 0),       0,   "Green Down"   },
    { IR_DOWNB, CRGB(0, 0, 0),       0,   "Blue Down"    },
};
#endif

// ---------------------------------------------------------
// Native RMT Decoder Implementation
// ---------------------------------------------------------

// Tolerance in microseconds. The StickS3's IR demodulator output has slow
// edges and weak-signal jitter that shifts our detected transitions by
// 200-400us. The two NEC space target windows (560 "0" and 1690 "1") stay
// non-overlapping until margin reaches ~565us, so 500us is the largest safe
// tolerance. Anything tighter loses too many real frames.
#define NEC_DECODE_MARGIN 500

namespace
{
    #if USE_WS281X && ESP_IDF_VERSION_MAJOR < 5
    // Reserve legacy RMT channel 7 for IR receive on WS281x builds. LED output
    // uses channels from 0 upward, so the only impossible case is asking for
    // eight LED channels and a remote at the same time. (Only relevant on the
    // legacy IDF 4.x backend, where channels are named by index. driver_ng
    // allocates channels dynamically so there's no risk of collision.)
    static_assert(NUM_CHANNELS < 8, "WS281x plus remote exceeds available legacy ESP32 RMT channels");
    #endif

#if ESP_IDF_VERSION_MAJOR < 5
    constexpr int kRemoteRmtChannelIndex = 7;
#endif

    // Project-local symbol type matching both rmt_item32_t (legacy) and
    // rmt_symbol_word_t (driver_ng). Defining our own struct lets the parser
    // be driver-agnostic - we don't have to include either RMT header just
    // to declare our shared parser machinery, and the .cpp's IDF-version-
    // gated backend code can reinterpret_cast its native symbol type to
    // IrSymbol* before calling ParseNecFrame.
    struct IrSymbol
    {
        uint32_t duration0 : 15;
        uint32_t level0    : 1;
        uint32_t duration1 : 15;
        uint32_t level1    : 1;
    };
    static_assert(sizeof(IrSymbol) == 4, "IrSymbol must be 32 bits to match rmt_item32_t / rmt_symbol_word_t layout");

    bool MatchUs(uint32_t measured, uint32_t target)
    {
        return measured + NEC_DECODE_MARGIN >= target &&
               measured <= target + NEC_DECODE_MARGIN;
    }

    // Data-space classification is intentionally threshold-based rather than
    // using the same narrow +/- margin as the leader/mark checks. On the
    // StickS3's demodulated IR input we've observed valid NEC "1" spaces land
    // in the 2200-2300us range, which is outside the classic 1690 +/- 500us
    // window but still clearly separated from a logical "0" space. Using the
    // midpoint between 560us and 1690us keeps 0/1 windows non-overlapping
    // while tolerating this receiver's stretched spaces.
    bool ParseNecDataSpace(uint32_t measured, bool& bitValue)
    {
        constexpr uint32_t kZeroNominalUs = 560;
        constexpr uint32_t kOneNominalUs  = 1690;
        constexpr uint32_t kMidpointUs    = (kZeroNominalUs + kOneNominalUs) / 2; // 1125us
        constexpr uint32_t kMinZeroUs     = 100;
        constexpr uint32_t kMaxOneUs      = 2600;

        if (measured >= kMidpointUs && measured <= kMaxOneUs)
        {
            bitValue = true;
            return true;
        }
        if (measured >= kMinZeroUs && measured < kMidpointUs)
        {
            bitValue = false;
            return true;
        }
        return false;
    }

    // Wider tolerance for the NEC leader specifically. The 9000us leader mark
    // and 4500us start-space (or 2250us repeat-space) are isolated wide
    // pulses with no neighboring bit windows to collide with, so we can be
    // much more generous than the +/-500us NEC_DECODE_MARGIN that the bit
    // decoder uses. Cheap 24-key remotes with weak/distant signal can shift
    // the leader by 800us or more (observed on the M5StickS3 receiver), and
    // tightening these margins back down loses too many real frames.
    bool MatchLeaderMarkUs(uint32_t measured)
    {
        return measured + 1500 >= 9000 && measured <= 9000 + 1500;   // 7500..10500us
    }
    bool MatchLeaderStartSpaceUs(uint32_t measured)
    {
        return measured + 1000 >= 4500 && measured <= 4500 + 1000;   // 3500..5500us
    }
    bool MatchLeaderRepeatSpaceUs(uint32_t measured)
    {
        return measured + 750 >= 2250 && measured <= 2250 + 750;     // 1500..3000us
    }

    // Decode a buffer of RMT symbols as an NEC frame. Returns true on success
    // and writes the 32-bit code or sets isRepeat. The MSB-first assembly is
    // intentional: the command tables in this file were authored against
    // those numeric values, and switching to protocol-pure LSB-first changes
    // every key code and breaks matching on working remotes.
    //
    // Noise-tolerant leader search: the StickS3's IR receiver occasionally
    // captures a stray transition just before the real NEC frame begins, so
    // a textbook frame ends up at symbol 1 or 2 instead of symbol 0. We
    // scan the first kLeaderSearchSymbols symbols for a pair that looks
    // like a NEC leader (~9000us mark + ~4500us start space, OR ~9000us
    // mark + ~2250us repeat space) and parse from whichever offset lands
    // it. If no plausible leader appears, the frame is rejected.
    bool ParseNecFrame(const IrSymbol* items, size_t count, uint32_t& code, bool& isRepeat)
    {
        isRepeat = false;

        // Find the symbol that holds a NEC leader (~9000us mark followed by
        // ~4500us start space or ~2250us repeat space). Returns count if
        // none is found. Search depth is generous because cheap remotes
        // sometimes spray several short noise symbols ahead of the real
        // leader, especially when the previous frame's tail bleeds past the
        // 20ms idle gap and gets stitched onto the front of this capture.
        constexpr size_t kLeaderSearchSymbols = 8;
        size_t leaderSymbol = count;
        for (size_t s = 0; s < count && s < kLeaderSearchSymbols; ++s)
        {
            if (MatchLeaderMarkUs(items[s].duration0) &&
                (MatchLeaderStartSpaceUs(items[s].duration1) ||
                 MatchLeaderRepeatSpaceUs(items[s].duration1)))
            {
                leaderSymbol = s;
                break;
            }
        }

        // The rest of the parser walks half-symbols (duration0 then
        // duration1 of each symbol) starting at the leader. getTime maps
        // a half-symbol index relative to the leader to its absolute
        // duration in the items array.
        auto getTime = [&](int relativeHalfIndex) -> uint32_t {
            const size_t absHalf = leaderSymbol * 2 + static_cast<size_t>(relativeHalfIndex);
            const size_t idx = absHalf / 2;
            return (absHalf & 1) ? items[idx].duration1 : items[idx].duration0;
        };

        const size_t symbolCount = (count - leaderSymbol) * 2;

        if (leaderSymbol == count)
        {
            return false;
        }

        if (symbolCount < 2)
        {
            return false;
        }

        // Leader mark already validated by the search loop; this redundant
        // check stays as defense-in-depth and to keep the diagnostic in
        // place if the search heuristics are tuned later.
        if (!MatchLeaderMarkUs(getTime(0)))
        {
            return false;
        }

        if (MatchLeaderRepeatSpaceUs(getTime(1)))
        {
            isRepeat = true;
            return true;
        }
        if (!MatchLeaderStartSpaceUs(getTime(1)))
        {
            return false;
        }

        // The project has historically used two families of NEC-like remotes:
        // the common 24-key / 44-key LED remotes whose published command
        // values are 24 bits wide (e.g. 0xF7C03F), and full 32-bit NEC remotes.
        // The old IRremote-based path handled both; the native parser should
        // do the same. A 24-bit frame needs 1 leader + 24 data symbols
        // (plus an optional trailer), while a 32-bit frame needs 1 leader +
        // 32 data symbols. In half-symbol terms that's 50 and 66 durations
        // respectively before any trailing mark.
        size_t bitCount = 0;
        if (symbolCount >= 66)
            bitCount = 32;
        else if (symbolCount >= 50)
            bitCount = 24;

        if (bitCount == 0)
        {
            return false;
        }

        uint32_t data = 0;
        for (size_t bitIndex = 0; bitIndex < bitCount; ++bitIndex)
        {
            const size_t i = 2 + bitIndex * 2;
            if (!MatchUs(getTime(i), 560))
            {
                return false;
            }

            data <<= 1;

            // 560us space = '0', 1690us space = '1'. Use a threshold-based
            // classifier because the S3 receiver can stretch logical '1'
            // spaces well past 2ms while still leaving a wide gap above '0'.
            bool bitValue = false;
            if (ParseNecDataSpace(getTime(i + 1), bitValue))
            {
                if (bitValue)
                    data |= 1;
            }
            else
            {
                return false;
            }
        }

        code = data;
        isRepeat = false;
        return true;
    }
}

// ---------------------------------------------------------
// Abstract base
// ---------------------------------------------------------
//
// RemoteControlImpl is the polymorphic interface that the public
// RemoteControl wrapper holds via unique_ptr. Concrete subclasses below
// pick which IDF RMT API to drive. The shared NEC parser machinery in
// the anonymous namespace above is invoked the same way from both.

class RemoteControlImpl
{
public:
    virtual ~RemoteControlImpl()                                = default;
    virtual bool        begin()                                 = 0;
    virtual bool        decode(uint32_t& code, bool& isRepeat)  = 0;
    virtual const char* DriverName() const                      = 0;
};

// ---------------------------------------------------------
// Legacy IDF RMT path (IDF 4.x)
// ---------------------------------------------------------
//
// Uses driver/rmt.h - rmt_config / rmt_driver_install / xRingbuffer.
// 100us hardware glitch filter via filter_ticks_thresh keeps short EMI
// pulses out of the symbol stream before the parser ever sees them.
//
// Gated on IDF 4.x because the legacy and new RMT headers cannot be
// included in the same translation unit (they both typedef the name
// rmt_channel_t to incompatible types). On IDF 5.x the factory below
// instantiates DriverNgRemoteControlImpl instead.

#if ESP_IDF_VERSION_MAJOR < 5

class LegacyRemoteControlImpl : public RemoteControlImpl
{
public:
    explicit LegacyRemoteControlImpl(int pin)
        : _pin(pin), _channel(static_cast<rmt_channel_t>(kRemoteRmtChannelIndex)) {}

    ~LegacyRemoteControlImpl() override
    {
        detachInterrupt(_pin);
        if (_begun)
        {
            rmt_rx_stop(_channel);
            rmt_driver_uninstall(_channel);
        }
    }

    bool begin() override
    {
        rmt_config_t config = RMT_DEFAULT_CONFIG_RX((gpio_num_t)_pin, _channel);
        config.clk_div = 80;                          // 80MHz / 80 = 1MHz resolution (1us per tick)
        config.rx_config.filter_en           = true;
        config.rx_config.filter_ticks_thresh = 100;   // Ignore pulses shorter than 100us
        config.rx_config.idle_threshold      = 20000; // 20ms idle = end of frame

        if (rmt_config(&config) != ESP_OK) return false;
        if (rmt_driver_install(_channel, 1024, ESP_INTR_FLAG_IRAM) != ESP_OK) return false;
        if (rmt_get_ringbuf_handle(_channel, &_ringbuf) != ESP_OK) return false;
        if (rmt_rx_start(_channel, true) != ESP_OK) return false;

        _begun = true;
        return true;
    }

    bool decode(uint32_t& code, bool& isRepeat) override
    {
        if (!_begun) return false;

        size_t        size  = 0;
        rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(_ringbuf, &size, 0);
        if (items)
        {
            // rmt_item32_t and IrSymbol share the same 32-bit bitfield
            // layout (verified by static_assert on sizeof). Reinterpret
            // is safe and lets the shared parser stay driver-agnostic.
            static_assert(sizeof(rmt_item32_t) == sizeof(IrSymbol),
                          "rmt_item32_t and IrSymbol must share size");
            const bool success = ParseNecFrame(reinterpret_cast<const IrSymbol*>(items),
                                               size / sizeof(rmt_item32_t),
                                               code, isRepeat);
            vRingbufferReturnItem(_ringbuf, items);
            return success;
        }
        return false;
    }

    const char* DriverName() const override { return "RMT Legacy"; }

private:
    int             _pin;
    rmt_channel_t   _channel;
    RingbufHandle_t _ringbuf = nullptr;
    bool            _begun   = false;
};

#endif // ESP_IDF_VERSION_MAJOR < 5

// ---------------------------------------------------------
// driver_ng RMT path (IDF 5.x only)
// ---------------------------------------------------------
//
// Uses driver/rmt_rx.h - rmt_new_rx_channel + rmt_rx_register_event_callbacks
// + a FreeRTOS queue between the on_recv_done ISR and the polling task.
// Required on Arduino-ESP32 3.x because the framework whole-archive-links
// libesp_driver_rmt.a; mixing in the legacy driver triggers a conflict
// ctor that aborts boot.
//
// Loses the 100us hardware filter (driver_ng caps at ~3us via
// signal_range_min_ns), so the parser's wider leader tolerances and
// midpoint bit classifier carry more weight here.

#if ESP_IDF_VERSION_MAJOR >= 5

namespace
{
    constexpr size_t   kIrSymbolBufferSize  = 96;          // enough for a full NEC frame + leader
    constexpr uint32_t kIrTickResolutionHz  = 1'000'000;   // 1us per tick

    struct IrFrame
    {
        rmt_symbol_word_t symbols[kIrSymbolBufferSize];
        size_t            count;
    };

    // rmt_receive_config_t embeds a union; designated-initializer support
    // for the named fields without naming the union is uneven across
    // GCC C++ modes, so build the struct field-by-field.
    inline rmt_receive_config_t MakeIrReceiveConfig()
    {
        rmt_receive_config_t cfg = {};
        cfg.signal_range_min_ns = 3000;             // ~3us glitch filter (HW max ~3187ns)
        cfg.signal_range_max_ns = 20 * 1000 * 1000; // 20ms idle -> end of frame
        return cfg;
    }
}

class DriverNgRemoteControlImpl : public RemoteControlImpl
{
public:
    explicit DriverNgRemoteControlImpl(int pin) : _pin(pin)
    {
        _frameQueue = xQueueCreate(4, sizeof(IrFrame));

        // rmt_receive in IDF 5.x rejects buffers that aren't in internal
        // RAM (esp_ptr_internal() check returns false for PSRAM). With
        // our PSRAM-by-default allocator this whole class instance lives
        // in PSRAM, so the receive buffer has to be allocated separately
        // with MALLOC_CAP_INTERNAL.
        _activeBuffer = static_cast<rmt_symbol_word_t*>(
            heap_caps_malloc(sizeof(rmt_symbol_word_t) * kIrSymbolBufferSize,
                             MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        if (_activeBuffer)
            std::fill_n(_activeBuffer, kIrSymbolBufferSize, rmt_symbol_word_t{});
    }

    ~DriverNgRemoteControlImpl() override
    {
        if (_begun && _channel)
        {
            rmt_disable(_channel);
            rmt_del_channel(_channel);
        }
        if (_frameQueue)   vQueueDelete(_frameQueue);
        if (_activeBuffer) heap_caps_free(_activeBuffer);
    }

    bool begin() override
    {
        if (!_frameQueue || !_activeBuffer) return false;

        // TSOP-style IR receivers idle high; without an explicit pull-up
        // the line floats between the demodulator's drives and ambient
        // EMI couples in enough to fire the on_recv_done callback on
        // 75-200us noise blips. Internal pull-up is enough to hold idle.
        gpio_set_pull_mode(static_cast<gpio_num_t>(_pin), GPIO_PULLUP_ONLY);

        rmt_rx_channel_config_t channelConfig = {};
        channelConfig.gpio_num         = static_cast<gpio_num_t>(_pin);
        channelConfig.clk_src          = RMT_CLK_SRC_DEFAULT;
        channelConfig.resolution_hz    = kIrTickResolutionHz;
        channelConfig.mem_block_symbols = 192;     // 4x S3 RAM blocks; reduces ISR pressure
        channelConfig.flags.invert_in   = false;
        channelConfig.flags.with_dma    = false;

        if (rmt_new_rx_channel(&channelConfig, &_channel) != ESP_OK) return false;

        rmt_rx_event_callbacks_t cbs = {};
        cbs.on_recv_done = OnRecvDoneTrampoline;
        if (rmt_rx_register_event_callbacks(_channel, &cbs, this) != ESP_OK) return false;

        if (rmt_enable(_channel) != ESP_OK) return false;

        // Initial arm. Subsequent re-arms happen from the ISR callback,
        // or as a defensive fallback in decode() when the ISR was forced
        // to skip its rearm (queue-full).
        const rmt_receive_config_t rxConfig = MakeIrReceiveConfig();
        if (rmt_receive(_channel, _activeBuffer,
                        sizeof(rmt_symbol_word_t) * kIrSymbolBufferSize,
                        &rxConfig) != ESP_OK)
            return false;

        _begun = true;
        return true;
    }

    bool decode(uint32_t& code, bool& isRepeat) override
    {
        if (!_begun || !_frameQueue) return false;

        IrFrame frame;
        if (xQueueReceive(_frameQueue, &frame, 0) != pdTRUE)
        {
            // No frame waiting. The ISR re-arms after every successful
            // queue send, so the channel is normally in-flight. The
            // recovery path here only fires when the ISR had to skip
            // its rearm (queue-full).
            if (_needsRearm.exchange(false, std::memory_order_acq_rel))
                ArmReceive();
            return false;
        }

        // rmt_symbol_word_t and our project-local IrSymbol have the same
        // 32-bit bitfield layout { duration0:15, level0:1, duration1:15, level1:1 }.
        // Reinterpreting lets the shared parser stay typed against IrSymbol
        // and free of any RMT-driver header dependency.
        static_assert(sizeof(rmt_symbol_word_t) == sizeof(IrSymbol),
                      "rmt_symbol_word_t and IrSymbol must share size");
        return ParseNecFrame(reinterpret_cast<const IrSymbol*>(frame.symbols),
                             frame.count, code, isRepeat);
    }

    const char* DriverName() const override { return "RMT driver_ng"; }

private:
    bool ArmReceive()
    {
        const rmt_receive_config_t rxConfig = MakeIrReceiveConfig();
        return rmt_receive(_channel, _activeBuffer,
                           sizeof(rmt_symbol_word_t) * kIrSymbolBufferSize,
                           &rxConfig) == ESP_OK;
    }

    // Called from the IDF's RMT RX ISR. Copies the captured symbols into
    // a stack frame and queues them for the polling task. If the queue
    // is full we drop the frame and leave _needsRearm set so decode()
    // recovers the channel state on its next poll. We also re-arm the
    // channel from here (rmt_receive uses ESP_RETURN_ON_FALSE_ISR
    // internally so it's ISR-safe per the IDF docs); doing it here
    // closes the dead window that otherwise existed between the
    // callback firing and decode() running.
    //
    // Not marked IRAM_ATTR: the function references too many flash
    // literals for the literal pool to fit adjacent to the IRAM
    // section, and IR RX doesn't run during flash ops anyway.
    static bool OnRecvDoneTrampoline(rmt_channel_handle_t channel,
                                     const rmt_rx_done_event_data_t* edata,
                                     void* userCtx)
    {
        auto*      self   = static_cast<DriverNgRemoteControlImpl*>(userCtx);
        BaseType_t higher = pdFALSE;

        if (self->_frameQueue && edata && edata->received_symbols)
        {
            IrFrame      frame;
            const size_t count = edata->num_symbols < kIrSymbolBufferSize
                                     ? edata->num_symbols
                                     : kIrSymbolBufferSize;
            for (size_t i = 0; i < count; ++i)
                frame.symbols[i] = edata->received_symbols[i];
            frame.count = count;

            if (xQueueSendFromISR(self->_frameQueue, &frame, &higher) == pdTRUE)
            {
                const rmt_receive_config_t rxConfig = MakeIrReceiveConfig();
                rmt_receive(channel, self->_activeBuffer,
                            sizeof(rmt_symbol_word_t) * kIrSymbolBufferSize,
                            &rxConfig);
            }
            else
            {
                // Queue full: drop frame, skip rearm, mark for recovery.
                self->_needsRearm.store(true, std::memory_order_release);
            }
        }

        return higher == pdTRUE;
    }

    int                  _pin;
    rmt_channel_handle_t _channel      = nullptr;
    QueueHandle_t        _frameQueue   = nullptr;
    rmt_symbol_word_t*   _activeBuffer = nullptr;
    bool                 _begun        = false;
    std::atomic<bool>    _needsRearm{false};
};

#endif // ESP_IDF_VERSION_MAJOR >= 5

// ---------------------------------------------------------
// Backend factory
// ---------------------------------------------------------
//
// Picks the right concrete impl based on IDF version. Localizes the only
// #if remaining outside the class definitions themselves.

namespace
{
    std::unique_ptr<RemoteControlImpl> MakeRemoteControlImpl(int pin)
    {
#if ESP_IDF_VERSION_MAJOR >= 5
        return std::make_unique<DriverNgRemoteControlImpl>(pin);
#else
        return std::make_unique<LegacyRemoteControlImpl>(pin);
#endif
    }
}

// ---------------------------------------------------------
// RemoteControl Wrappers
// ---------------------------------------------------------

RemoteControl::RemoteControl() : _pImpl(MakeRemoteControlImpl(IR_REMOTE_PIN)) {}
RemoteControl::~RemoteControl() { Stop(); }

// begin
//
// Sets up the IR receiver hardware. Called from Run() once ITaskService::Start
// has launched the polling task on its own FreeRTOS thread. The driver name
// printed here comes from the concrete impl (Legacy / driver_ng), so it's
// easy to confirm which RMT path the running build is using.

bool RemoteControl::begin() {
    debugW("Native Remote Control Decoding Started (%s)",
           _pImpl ? _pImpl->DriverName() : "no impl");
    return _pImpl ? _pImpl->begin() : false;
}

// end
//
// Public-facing teardown. Used by the OTA "onStart" hook to make sure the IR
// polling task isn't competing with the firmware update for CPU/RMT
// resources. Delegates to the inherited ITaskService::Stop() so the
// shutdown is the same idempotent path used during normal service teardown.

void RemoteControl::end() {
    debugW("Native Remote Control Decoding Stopped");
    Stop();
}

// ITaskService hooks
//
// Start/Stop/IsRunning are inherited final from ITaskService; this class only
// supplies the task config and the IR polling loop body.

ITaskService::TaskConfig RemoteControl::GetTaskConfig() const
{
    return TaskConfig {
        "IR Remote Loop",
        REMOTE_STACK_SIZE,
        REMOTE_PRIORITY,
        REMOTE_CORE,
        250    // Stop timeout: polling loop yields every 20ms.
    };
}

// RemoteControl::Run
//
// Initializes the IR receiver and then polls for codes every 20ms until
// ShouldShutdown() is true. ITaskService::TaskEntryThunk handles the
// post-Run vTaskDelete and running-state bookkeeping when this returns.
void RemoteControl::Run()
{
    debugW("RemoteControl::Run entered (ENABLE_REMOTE=%d, IR_REMOTE_PIN=%d)",
           static_cast<int>(ENABLE_REMOTE), static_cast<int>(IR_REMOTE_PIN));

    const bool ok = begin();
    debugW("RemoteControl::Run: begin() returned %s; entering poll loop",
           ok ? "true" : "false");

    while (!ShouldShutdown())
    {
        handle();
        delay(20);
    }

    debugW("RemoteControl::Run: ShouldShutdown() true, exiting");
}

#define BRIGHTNESS_STEP     20

// handle
//
// Main function for the remote control task.  It checks for new remote codes and then takes the
// appropriate action based on the code received.

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

    auto &effectManager = g_ptrSystem->GetEffectManager();
    auto &deviceConfig = g_ptrSystem->GetDeviceConfig();

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

            // Only apply color if it's not Black (used as placeholder for command keys)
            if (RemoteColorCode.color != CRGB::Black)
            {
                effectManager.ApplyGlobalColor(RemoteColorCode.color);
                #if FULL_COLOR_REMOTE_FILL
                    auto effect = std::make_shared<ColorFillEffect>("Remote Color", RemoteColorCode.color, 1, true);
                    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);
                    if (effect->Init(g_ptrSystem->GetEffectManager().GetBaseGraphics()))
                        g_ptrSystem->GetEffectManager().SetTempEffect(effect);
                    else
                        debugE("Could not initialize new color fill effect");
                #endif
            }
            return;
        }
    }

    // Log unknown codes
    debugD("Remote: Unknown Code 0x%08lX", (unsigned long)result);
}

#endif
