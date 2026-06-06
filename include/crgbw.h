#pragma once

//+--------------------------------------------------------------------------
//
// File:        crgbw.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//   CRGBW: a two-byte "whites plane" pixel type used alongside the existing
//   FastLED CRGB array for 4- and 5-channel addressable LED strips.
//
//   - cw  = cool-white channel intent (0..255)
//   - ww  = warm-white channel intent (0..255)
//
//   The internal representation always carries both channels regardless of
//   what the physical strip can render. The PixelFormat for each chip
//   (Ws2812Format / Sk6812Format / Sm16825Format) decides how to map
//   (cw, ww) to that chip's actual channel count:
//
//     - 3-channel WS2812:  whites are ignored (or folded back into RGB by
//                          the format's policy; default is "discard").
//     - 4-channel SK6812:  the strip has one physical white at a fixed
//                          color temperature; the format combines cw + ww
//                          into a single W output value. Kelvin is intent
//                          metadata on that path, not independently rendered.
//     - 5-channel SM16825: cw and ww are emitted directly as separate
//                          channels.
//
//   Effects that don't care about CCT continue to write only to leds[];
//   the per-pixel shared-portion synthesis in the PixelFormat fills in
//   the white channels automatically. Effects that want explicit control
//   call setPixelWhite() or setPixelCCT() on GFXBase to write the whites
//   plane directly.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <algorithm>
#include <cstdint>

struct CRGBW
{
    uint8_t cw;     // cool-white channel intent (0..255)
    uint8_t ww;     // warm-white channel intent (0..255)

    constexpr CRGBW() noexcept : cw(0), ww(0) {}
    constexpr CRGBW(uint8_t cool, uint8_t warm) noexcept : cw(cool), ww(warm) {}

    constexpr bool isZero() const noexcept { return cw == 0 && ww == 0; }

    static constexpr CRGBW Black() noexcept { return CRGBW(0, 0); }
};

static_assert(sizeof(CRGBW) == 2, "CRGBW must be 2 bytes for compact whites-plane storage");

// SplitByCct
//
// Maps a desired (kelvin, brightness) pair to a (cw, ww) intensity pair.
// Linear interpolation between 2700K (pure warm) and 6500K (pure cool):
//
//   kelvin = 2700K -> (cw=0,   ww=brightness)
//   kelvin = 4600K -> (cw=brightness/2, ww=brightness/2)
//   kelvin = 6500K -> (cw=brightness, ww=0)
//
// Below 2700K clamps to pure WW; above 6500K clamps to pure CW.
//
// This linear curve is a first cut. Real LED CW/WW phosphors don't have a
// clean blackbody curve and the perceived neutral point depends on the
// strip vendor. Once we have a 5-channel strip in hand we'll replace this
// with a tunable LUT (or per-strip calibration) and ship that as a
// separate change.
//
// We deliberately keep the helper inline + header-only so the PixelFormat
// implementations can call it from per-frame hot paths without a function
// call hop.

inline CRGBW SplitByCct(uint16_t kelvin, uint8_t brightness) noexcept
{
    constexpr uint16_t kKelvinWarm = 2700;
    constexpr uint16_t kKelvinCool = 6500;
    constexpr uint16_t kKelvinSpan = kKelvinCool - kKelvinWarm; // 3800

    if (brightness == 0)
        return CRGBW::Black();

    const uint16_t clamped = std::min<uint16_t>(std::max<uint16_t>(kelvin, kKelvinWarm), kKelvinCool);
    const uint16_t coolPart = static_cast<uint16_t>(clamped - kKelvinWarm); // 0 .. 3800
    // cw fraction = (kelvin - 2700) / (6500 - 2700)
    // ww fraction = 1 - cw fraction
    const uint16_t cw16 = static_cast<uint16_t>((coolPart * brightness + (kKelvinSpan / 2)) / kKelvinSpan);
    const uint16_t ww16 = static_cast<uint16_t>(brightness - cw16);
    return CRGBW(static_cast<uint8_t>(cw16), static_cast<uint8_t>(ww16));
}
