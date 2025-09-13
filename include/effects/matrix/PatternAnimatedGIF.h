//+--------------------------------------------------------------------------
//
// File:        PatternAnimatedGIF.h
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
//   Displays embedded GIF animations on the LED matrix.  GIF files
//   are embedded in the flash image and are decoded on the fly.  The
//   GIF decoder is from Louis Beaudoin's GifDecoder library.  We use
//   that to extract frames from the GIF and then plot them on the
//   LED matrix.  We do that by supplying callbacks to the GIF decoder
//   that it calls to fetch the GIF data and to plot the pixels on the
//   LED matrix.
//
// History:     Nov-21-2023         Davepl      Created
//
//---------------------------------------------------------------------------

#ifndef PatternAnimatedGIF_H
#define PatternAnimatedGIF_H

#include <Arduino.h>
#include "globals.h"
#include <string.h>
#include <ledstripeffect.h>
#include <hub75gfx.h>
#include <ArduinoJson.h>
#include "systemcontainer.h"
#include <map>
#include "effects.h"
#include "types.h"
#include <GifDecoder.h>

// The GIF files are embedded within the flash image, and we need to tell the linker where they are

extern const uint8_t colorsphere_start[]     asm ("_binary_assets_gif_colorsphere_gif_start");
extern const uint8_t colorsphere_end[]       asm ("_binary_assets_gif_colorsphere_gif_end");
extern const uint8_t atomic_start[]          asm ("_binary_assets_gif_atomic_gif_start");
extern const uint8_t atomic_end[]            asm ("_binary_assets_gif_atomic_gif_end");
extern const uint8_t threerings_start[]      asm ("_binary_assets_gif_threerings_gif_start");
extern const uint8_t threerings_end[]        asm ("_binary_assets_gif_threerings_gif_end");
extern const uint8_t pacman_start[]          asm ("_binary_assets_gif_pacman_gif_start");
extern const uint8_t pacman_end[]            asm ("_binary_assets_gif_pacman_gif_end");
extern const uint8_t banana_start[]          asm ("_binary_assets_gif_banana_gif_start");
extern const uint8_t banana_end[]            asm ("_binary_assets_gif_banana_gif_end");
extern const uint8_t nyancat_start[]         asm ("_binary_assets_gif_nyancat_gif_start");
extern const uint8_t nyancat_end[]           asm ("_binary_assets_gif_nyancat_gif_end");
extern const uint8_t tesseract_start[]       asm ("_binary_assets_gif_tesseract_gif_start");
extern const uint8_t tesseract_end[]         asm ("_binary_assets_gif_tesseract_gif_end");
extern const uint8_t firelog_start[]         asm ("_binary_assets_gif_firelog_gif_start");
extern const uint8_t firelog_end[]           asm ("_binary_assets_gif_firelog_gif_end");

// AnimatedGIFs
//
// Our set of embedded GIFs.  Currently assumed to be 32x32 in size, default FPS.

enum class GIFIdentifier : int
{
    INVALID     = 0,
    Atomic      = 1,
    ColorSphere = 2,
    Pacman      = 3,
    ThreeRings  = 4,
    Banana      = 5,
    Tesseract   = 6,
    Nyancat     = 7,
    Firelog     = 8
};

// GIFInfo
//
// Extended "EmbeddedFile" that also tracks the width and height of the GIF

struct GIFInfo : public EmbeddedFile
{
    uint16_t _width;
    uint16_t _height;
    uint8_t _fps;
    GIFInfo(const uint8_t start[], const uint8_t end[], uint16_t width, uint16_t height, uint8_t fps)
        : EmbeddedFile(start, end), _width(width), _height(height), _fps(fps)
    {
    }
};

static const std::map<GIFIdentifier, const GIFInfo, std::less<GIFIdentifier>, const psram_allocator<std::pair<GIFIdentifier, const GIFInfo> > > AnimatedGIFs =
{
    // Banana has 8 frames.  Most music is around 120BPM, so we need to play each frame for 1/15th of a second to somewhat align with a typical beat
    { GIFIdentifier::Banana,       GIFInfo(banana_start,      banana_end,      32, 32, 10 ) },      //  4 KB
    { GIFIdentifier::Nyancat,      GIFInfo(nyancat_start,     nyancat_end,     64, 32, 18 ) },      // 20 KB
    { GIFIdentifier::Pacman,       GIFInfo(pacman_start,      pacman_end,      64, 12, 20 ) },      // 36 KB
    { GIFIdentifier::Atomic,       GIFInfo(atomic_start,      atomic_end,      32, 32, 60 ) },      // 21 KB
    { GIFIdentifier::ColorSphere,  GIFInfo(colorsphere_start, colorsphere_end, 32, 32, 16 ) },      // 52 KB
    { GIFIdentifier::ThreeRings,   GIFInfo(threerings_start,  threerings_end,  64, 32, 24 ) },      //  9 KB
    { GIFIdentifier::Tesseract,    GIFInfo(tesseract_start,   tesseract_end,   40, 32, 40 ) },      // 24 KB
    { GIFIdentifier::Firelog,      GIFInfo(firelog_start,     firelog_end,     64, 32, 16 ) },      // 24 KB
};

// The decoder needs us to track some state, but there's only one instance of the decoder, and
// we can't pass it a pointer to our state because the callback doesn't allow you to pass any
// context, and you can't use a lambda that captures the this pointer because that can't be
// converted to a callback function pointer.  So we have to use a global.

struct
{
    int _offsetX   = 0;
    int _offsetY   = 0;
    uint8_t _fps       = 24;
    CRGB _bkColor   = CRGB::Black;
    // Scaling parameters for best-fit rendering
    float _scaleX    = 1.0f;
    float _scaleY    = 1.0f;
    uint16_t _srcWidth  = 0;
    uint16_t _srcHeight = 0;
    uint16_t _dstWidth  = 0;
    uint16_t _dstHeight = 0;
}
g_gifDecoderState;

// We dynamically allocate the GIF decoder because it's pretty big and we don't want to waste the base
// ram on it.  This way it, and the GIFs it decodes, can live in PSRAM.

const std::unique_ptr<GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, 16, true> > g_ptrGIFDecoder = make_unique_psram<GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, 16, true> >();

// PatternAnimatedGIF
//
// Draws a cycling animated GIF on the LED matrix.  Use GifDecoder to do the heavy lifting behind the scenes.

class PatternAnimatedGIF : public EffectWithId<PatternAnimatedGIF>
{
  private:

    GIFIdentifier _gifIndex  = GIFIdentifier::INVALID;
    CRGB _bkColor            = BLACK16;
    bool _preClear           = false;
    bool _gifReadyToDraw     = false;

    // GIF decoder callbacks.  These are static because the decoder doesn't allow you to pass any context, so they
    // have to be global.  We use the global g_gifDecoderState to track state.  The GifDecoder code calls back to
    // these callbacks to do the actual work of plotting them on the LED matrix.

    // screenClearCallback - clears the screen with the color given to the constructor

    static void screenClearCallback(void)
    {
        auto& g = *(g_ptrSystem->EffectManager().g());
        g.Clear(g_gifDecoderState._bkColor);
    }

    // We decide when to update the screen, so this is a no-op

    static void updateScreenCallback(void)
    {
        debugV("UpdateScreenCallback from AnimatedGIF decoder.");
    }

    // drawPixelCallback
    //
    // This is called by the GIF decoder to draw a pixel.  We use scaling and offset to fit the GIF on the LED matrix.

    static void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue)
    {
        auto& g = *(g_ptrSystem->EffectManager().g(0));

        // Apply scaling transformation
        int16_t scaledX = (int16_t)(x * g_gifDecoderState._scaleX) + g_gifDecoderState._offsetX;
        int16_t scaledY = (int16_t)(y * g_gifDecoderState._scaleY) + g_gifDecoderState._offsetY;

        if (false == g.isValidPixel(scaledX, scaledY))
        {
            debugV("drawPixelCallback: scaled pixel out of bounds: %d, %d (from source %d, %d)", scaledX, scaledY, x, y);
            return;
        }

        // If we're scaling down (scale < 1.0), we might want to sample multiple source pixels
        // For now, we use simple nearest-neighbor scaling
        g.leds[XY(scaledX, scaledY)] = CRGB(red, green, blue);
    }

    // drawLineCallback
    //
    // This is called by the GIF decoder to draw a line of pixels.

    static void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip)
    {
        // I don't think this is ever called, but if it is, we may need to implement it.  For now, it seems they just
        // call drawPixelCallback for each pixel in the image.

        throw new std::runtime_error("drawLineCallback not implemented for animated GIFs");
    }

    // For slower animations that run at a lower framerate, we double the framerate by discarding every other frame,
    // which allows us to draw the VU meter and so on at a useable rate even though the animation doesn't paint every time.

    static bool FrameDoubling()
    {
        return g_gifDecoderState._fps <= 15;
    }

    size_t DesiredFramesPerSecond() const override
    {
        return FrameDoubling() ? g_gifDecoderState._fps * 2 : g_gifDecoderState._fps;
    }

  public:

    PatternAnimatedGIF(const String & friendlyName, GIFIdentifier gifIndex, bool preClear = false, CRGB bkColor = CRGB::Black)
        : EffectWithId<PatternAnimatedGIF>(friendlyName),
        _preClear(preClear),
        _gifIndex(gifIndex),
        _bkColor(bkColor)
    {
    }

    PatternAnimatedGIF(const JsonObjectConst& jsonObject)
        : EffectWithId<PatternAnimatedGIF>(jsonObject),
        _preClear(jsonObject[PTY_PRECLEAR]),
        _gifIndex((GIFIdentifier)jsonObject[PTY_GIFINDEX].as<std::underlying_type_t<GIFIdentifier> >()),
        _bkColor(jsonObject[PTY_BKCOLOR])
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_GIFINDEX]  = to_value(_gifIndex);
        jsonDoc[PTY_BKCOLOR]   = _bkColor;
        jsonDoc[PTY_PRECLEAR]  = _preClear;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Start() override
    {
        g()->Clear(_bkColor);

        // Open the GIF and start decoding

        auto gif = AnimatedGIFs.find(_gifIndex);
        if (gif == AnimatedGIFs.end())
            throw std::runtime_error(str_sprintf("Unable to locate GIF by index %d in the map.", (int) _gifIndex).c_str());

        // Set up the gifDecoderState with all of the context that it will need to decode and
        // draw the GIF, since the static callbacks will have no other context to work with.

        // Calculate best-fit scaling if the GIF is larger than the matrix
        uint16_t gifWidth = gif->second._width;
        uint16_t gifHeight = gif->second._height;

        float scaleX = 1.0f;
        float scaleY = 1.0f;

        // If GIF is larger than matrix, calculate scaling to fit
        if (gifWidth > MATRIX_WIDTH || gifHeight > MATRIX_HEIGHT)
        {
            scaleX = (float)MATRIX_WIDTH / (float)gifWidth;
            scaleY = (float)MATRIX_HEIGHT / (float)gifHeight;

            // Use the smaller scale factor to maintain aspect ratio (best fit)
            float scale = min(scaleX, scaleY);
            scaleX = scale;
            scaleY = scale;
        }

        // Calculate the destination dimensions after scaling
        uint16_t dstWidth = (uint16_t)(gifWidth * scaleX);
        uint16_t dstHeight = (uint16_t)(gifHeight * scaleY);

        // Center the scaled GIF on the matrix
        int offsetX = (MATRIX_WIDTH - dstWidth) / 2;
        int offsetY = (MATRIX_HEIGHT - dstHeight) / 2;

        debugI("GIF scaling: %dx%d -> %dx%d (scale %.2f,%.2f) offset (%d,%d)",
                gifWidth, gifHeight, dstWidth, dstHeight, scaleX, scaleY, offsetX, offsetY);

        g_gifDecoderState._offsetX   = offsetX;
        g_gifDecoderState._offsetY   = offsetY;
        g_gifDecoderState._fps       = gif->second._fps;
        g_gifDecoderState._bkColor   = _bkColor;
        g_gifDecoderState._scaleX    = scaleX;
        g_gifDecoderState._scaleY    = scaleY;
        g_gifDecoderState._srcWidth  = gifWidth;
        g_gifDecoderState._srcHeight = gifHeight;
        g_gifDecoderState._dstWidth  = dstWidth;
        g_gifDecoderState._dstHeight = dstHeight;

        // Set the GIF decoder callbacks to our static functions

        g_ptrGIFDecoder->setScreenClearCallback( screenClearCallback );
        g_ptrGIFDecoder->setUpdateScreenCallback( updateScreenCallback );
        g_ptrGIFDecoder->setDrawPixelCallback( drawPixelCallback );
        g_ptrGIFDecoder->setDrawLineCallback( drawLineCallback );

        _gifReadyToDraw = (ERROR_NONE == g_ptrGIFDecoder->startDecoding((uint8_t *) gif->second.contents, gif->second.length));
        if (!_gifReadyToDraw)
            debugW("Failed to start decoding GIF");
    }

    void Draw() override
    {
        // If we're running a low FPS animation, we discard alternate frames and draw every other one, which allows
        // the VU meter to paint on every frame and remain responsive.

        static bool discardFrame = false;
        if (FrameDoubling())
        {
            discardFrame = !discardFrame;
            if (discardFrame)
                return;
        }

        // GIFs that use transparency will leave the previous frame in place, so we need
        // to clear the screen before we draw the next frame.  We can skip this if the
        // GIF doesn't use transparency.

        if (_preClear)
            g()->Clear(_bkColor);

        if (_gifReadyToDraw)
            g_ptrGIFDecoder->decodeFrame(false);

    }
};

#endif
