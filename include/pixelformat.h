#pragma once

//+--------------------------------------------------------------------------
//
// File:        pixelformat.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//   PixelFormat is the chip-specific policy that converts the framebuffer
//   pair (CRGB leds[], CRGBW whites[]) into the byte stream the addressable
//   LED chip on the wire actually expects.
//
//   The Transport class in ws281xoutputmanager.cpp owns the RMT plumbing
//   (legacy driver/rmt.h or IDF5 driver_ng) and is identical across chips
//   in the WS281x family. PixelFormat is the *orthogonal* axis: how many
//   bytes per pixel, and how (R, G, B, optional CW, optional WW) map onto
//   those bytes.
//
//   Concrete subclasses:
//
//     Ws2812Format   - 3 bytes/pixel.  Whites plane is ignored. Existing
//                      behavior; reproduces what PackChannelPixelsForColorOrder
//                      did before this strategy class existed.
//
//     Sk6812Format   - 4 bytes/pixel.  Strip has one physical white LED at
//                      a fixed color temperature (RGBW / RGBNW / RGBWW SKUs).
//                      W is synthesized from RGB shared-portion + summed
//                      with effect-explicit (cw, ww) from the whites plane.
//
//     [future] Sm16825Format / Ws2805Format - 5 bytes/pixel. CW and WW
//                      emitted directly as separate channels.
//
//   All formats currently use the same WS281x family bit timings (800 kHz
//   NRZ). Future 5-channel formats will need different timings; when that
//   happens we'll add T0H/T0L/T1H/T1L virtuals here and have the Transport
//   read them at channel-configure time. Not needed yet for SK6812.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "crgbw.h"          // CRGBW + SplitByCct helper
#include "deviceconfig.h"   // DeviceConfig::WS281xColorOrder
#include "pixeltypes.h"     // FastLED CRGB

// ---------------------------------------------------------------------
// Abstract base
// ---------------------------------------------------------------------

class PixelFormat
{
public:
    virtual ~PixelFormat() = default;

    // How many bytes does one packed pixel occupy on the wire? 3 for plain
    // RGB chips, 4 for RGBW, 5 for RGBCCW.
    virtual size_t BytesPerPixel() const = 0;

    // Pack `activeLedCount` pixels into `output`. Pixels at index >=
    // pixelsToShow are written as black. `whites` may be nullptr (which
    // is the normal case for plain CRGB effects) - in that case the
    // format's default synthesis policy is used to derive any white
    // channels from RGB content.
    //
    // brightness and fader are applied uniformly to every channel.
    //
    // cctKelvin is the global color-temperature target used to split a
    // synthesized white between cool-white and warm-white outputs.
    // ambientCw and ambientWw are per-pixel floors added under whatever
    // effects produce. Both are no-ops on a 3-channel format.
    //
    // whiteExtractRatio (0..255) controls how aggressively the shared-
    // portion white is *pulled out* of RGB into the dedicated white
    // channel(s) by the format's auto-synthesis path:
    //   - 0   = don't extract; the W LED stays off and RGB does all the
    //           additive white the way a WS2812 does
    //   - 255 = full extraction; the entire min(R,G,B) shared portion
    //           routes to W and RGB drops to (saturation-only) on the
    //           white-share region. This is what looks washed out on
    //           strips with bright W LEDs.
    //   - 128 = half-and-half (current default). Half the shared white
    //           routes to W (where the brighter dedicated LED carries
    //           it efficiently), half stays in RGB (preserves color
    //           character on warm/cool-tinted content).
    // Per-pixel explicit whites (effects calling setPixelWhite /
    // setPixelCCT) are NOT scaled by this - they're additive on top.
    virtual void Pack(uint8_t* output,
                      const CRGB* leds,
                      const CRGBW* whites,                       // may be nullptr
                      size_t activeLedCount,
                      size_t pixelsToShow,
                      uint8_t brightness,
                      uint8_t fader,
                      DeviceConfig::WS281xColorOrder colorOrder,
                      uint16_t cctKelvin,
                      uint8_t ambientCw,
                      uint8_t ambientWw,
                      uint8_t whiteExtractRatio) const = 0;
};

// ---------------------------------------------------------------------
// Helpers shared by concrete formats
// ---------------------------------------------------------------------

namespace PixelFormatHelpers
{
    // Apply brightness + fader video-scale to a single channel byte.
    inline uint8_t Scale(uint8_t value, uint8_t brightness, uint8_t fader)
    {
        // Mirrors nscale8_video twice; kept inline so the per-pixel inner
        // loop avoids function call overhead. Same math as FastLED's
        // nscale8x3_video applied per-channel.
        const uint16_t a = (static_cast<uint16_t>(value) * (static_cast<uint16_t>(brightness) + 1)) >> 8;
        const uint16_t b = (a * (static_cast<uint16_t>(fader) + 1)) >> 8;
        return static_cast<uint8_t>(b);
    }

    inline uint8_t SaturatingAdd(uint8_t a, uint8_t b)
    {
        const uint16_t s = static_cast<uint16_t>(a) + static_cast<uint16_t>(b);
        return s > 255 ? 255 : static_cast<uint8_t>(s);
    }

    // The three RGB output byte indices for a wire-side color order.
    struct ColorOrderIndices
    {
        uint8_t rIdx;
        uint8_t gIdx;
        uint8_t bIdx;
    };

    inline ColorOrderIndices IndicesFor(DeviceConfig::WS281xColorOrder order)
    {
        switch (order)
        {
            case DeviceConfig::WS281xColorOrder::RGB: return { 0, 1, 2 };
            case DeviceConfig::WS281xColorOrder::RBG: return { 0, 2, 1 };
            case DeviceConfig::WS281xColorOrder::GRB: return { 1, 0, 2 };
            case DeviceConfig::WS281xColorOrder::GBR: return { 2, 0, 1 };
            case DeviceConfig::WS281xColorOrder::BRG: return { 1, 2, 0 };
            case DeviceConfig::WS281xColorOrder::BGR: return { 2, 1, 0 };
            default:                                  return { 1, 0, 2 }; // GRB default
        }
    }
}

// ---------------------------------------------------------------------
// Ws2812Format - 3 bytes per pixel, whites ignored
// ---------------------------------------------------------------------
//
// Reproduces the pre-strategy behavior exactly. Picked as the format when
// the build doesn't enable white channels.

class Ws2812Format final : public PixelFormat
{
public:
    size_t BytesPerPixel() const override { return 3; }

    void Pack(uint8_t* output,
              const CRGB* leds,
              const CRGBW* /*whites*/,
              size_t activeLedCount,
              size_t pixelsToShow,
              uint8_t brightness,
              uint8_t fader,
              DeviceConfig::WS281xColorOrder colorOrder,
              uint16_t /*cctKelvin*/,
              uint8_t /*ambientCw*/,
              uint8_t /*ambientWw*/,
              uint8_t /*whiteExtractRatio*/) const override
    {
        // No W channel - shared-portion extraction has nowhere to route, so
        // the ratio knob is a no-op here. Plain RGB pack only.
        const auto idx = PixelFormatHelpers::IndicesFor(colorOrder);
        for (size_t i = 0; i < activeLedCount; ++i)
        {
            CRGB color = (i < pixelsToShow) ? leds[i] : CRGB::Black;

            uint8_t r = PixelFormatHelpers::Scale(color.r, brightness, fader);
            uint8_t g = PixelFormatHelpers::Scale(color.g, brightness, fader);
            uint8_t b = PixelFormatHelpers::Scale(color.b, brightness, fader);

            const size_t off = i * 3;
            output[off + idx.rIdx] = r;
            output[off + idx.gIdx] = g;
            output[off + idx.bIdx] = b;
        }
    }
};

// ---------------------------------------------------------------------
// Sk6812Format - 4 bytes per pixel (RGB + single white)
// ---------------------------------------------------------------------
//
// SK6812 strips have one physical white LED per pixel at a fixed color
// temperature (RGBW / RGBNW typically ~4000K; RGBWW ~3000K; RGBCW ~6500K).
// We collapse the project's internal (cw, ww) dual-white intent down to a
// single W byte by simply summing the two with saturating add. That treats
// "cool intent" and "warm intent" as equivalent calls for "light up the
// strip's white LED."
//
// For full CCT control we need a 5-channel chip (SM16825 / WS2805). The
// single-white case is the obvious degenerate.
//
// Wire byte order is the same color-order config as WS2812 for the RGB
// triple, plus W tacked on as byte 3. (BTF / Adafruit SK6812 RGBW strips
// ship with GRBW on the wire — DeviceConfig::WS281xColorOrder == GRB plus
// W in byte 3 reproduces that.)

class Sk6812Format final : public PixelFormat
{
public:
    size_t BytesPerPixel() const override { return 4; }

    void Pack(uint8_t* output,
              const CRGB* leds,
              const CRGBW* whites,
              size_t activeLedCount,
              size_t pixelsToShow,
              uint8_t brightness,
              uint8_t fader,
              DeviceConfig::WS281xColorOrder colorOrder,
              uint16_t /*cctKelvin*/,
              uint8_t ambientCw,
              uint8_t ambientWw,
              uint8_t whiteExtractRatio) const override
    {
        const auto idx = PixelFormatHelpers::IndicesFor(colorOrder);
        // Saturating-sum the ambient floor once outside the loop. Single
        // white LED can't reproduce CW/WW separately so we collapse here.
        const uint8_t ambientWhite = PixelFormatHelpers::SaturatingAdd(ambientCw, ambientWw);
        const uint16_t ratio = static_cast<uint16_t>(whiteExtractRatio); // 0..255

        for (size_t i = 0; i < activeLedCount; ++i)
        {
            CRGB color = (i < pixelsToShow) ? leds[i] : CRGB::Black;

            // Partial-extraction shared-portion subtract.
            //
            // The W LED on most SK6812 RGBW SKUs is noticeably brighter per
            // digital count than the RGB sum, so pulling the full shared
            // intensity into W and dropping RGB to (saturation-only) makes
            // any near-white content look washed out and pastel.
            //
            // Instead we pull `ratio/255` of the shared white into W and
            // leave the rest in the RGB triple. At ratio=0 the W LED stays
            // off and the pixel renders exactly like a WS2812 would; at
            // ratio=255 we fully transfer to W (old behavior); the default
            // of 128 strikes a workable middle ground on the strips we've
            // tested so far. The right value is strip-dependent; expose
            // through DeviceConfig + SetupUI once we wire that through.
            uint8_t effectWhite = 0;
            if (whites)
                effectWhite = PixelFormatHelpers::SaturatingAdd(whites[i].cw, whites[i].ww);

            // Explicit effect-set whites are additive on top of RGB. When an
            // effect wrote a white value, do not pull shared white out of RGB:
            // the effect is intentionally asking for both RGB and W output.
            uint8_t pull = 0;
            if (effectWhite == 0)
            {
                const uint8_t shared = std::min(color.r, std::min(color.g, color.b));
                // pull = round(shared * ratio / 255)
                pull = static_cast<uint8_t>((static_cast<uint16_t>(shared) * ratio + 127) / 255);
                color.r -= pull;
                color.g -= pull;
                color.b -= pull;
            }

            uint8_t w = PixelFormatHelpers::SaturatingAdd(pull, effectWhite);
            w        = std::max(w, ambientWhite);

            uint8_t r = PixelFormatHelpers::Scale(color.r, brightness, fader);
            uint8_t g = PixelFormatHelpers::Scale(color.g, brightness, fader);
            uint8_t b = PixelFormatHelpers::Scale(color.b, brightness, fader);
            uint8_t wOut = PixelFormatHelpers::Scale(w,     brightness, fader);

            const size_t off = i * 4;
            output[off + idx.rIdx] = r;
            output[off + idx.gIdx] = g;
            output[off + idx.bIdx] = b;
            output[off + 3]        = wOut;        // W always last byte
        }
    }
};
