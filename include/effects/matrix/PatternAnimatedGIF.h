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
#include <string.h>
#include <ledstripeffect.h>
#include <ledmatrixgfx.h>
#include <ArduinoJson.h>
#include "systemcontainer.h"
#include <map>
#include "effects.h"
#include "types.h"
#include <GifDecoder.h>

// The GIF files are embedded within the flash image, and we need to tell the linker where they are

extern const uint8_t atomic_start[]          asm("_binary_assets_gif_atomic_gif_start");
extern const uint8_t atomic_end[]            asm("_binary_assets_gif_atomic_gif_end");
extern const uint8_t colorsphere_start[]     asm("_binary_assets_gif_colorsphere_gif_start");
extern const uint8_t colorsphere_end[]       asm("_binary_assets_gif_colorsphere_gif_end");

// AnimatedGIFs
//
// Our set of embedded GIFs.  Currently assumed to be 32x32 in size, default FPS.

enum
{
    INVALID     = 0,
    Atomic      = 1,
    ColorSphere = 2
} GIFIdentifier;

static std::map<int, EmbeddedFile, std::less<int>, psram_allocator<std::pair<int, EmbeddedFile>>> AnimatedGIFs =
{
    { Atomic, EmbeddedFile(atomic_start, atomic_end) },
    { ColorSphere, EmbeddedFile(colorsphere_start, colorsphere_end) },
};

constexpr auto GIF_WIDTH = 32;
constexpr auto GIF_HEIGHT = 32;

static_assert(GIF_WIDTH <= MATRIX_WIDTH, "GIF_WIDTH must be <= MATRIX_WIDTH");
static_assert(GIF_HEIGHT <= MATRIX_HEIGHT, "GIF_HEIGHT must be <= MATRIX_HEIGHT");

// The decoder needs us to track some state, but there's only one instance of the decoder, and 
// we can't pass it a pointer to our state because the callback doesn't allow you to pass any
// context, and you can't use a lambda that captures the this pointer because that can't be
// converted to a callback function pointer.  So we have to use a global. 

struct 
{
    long _seek = 0;
    const uint8_t *_pgif = nullptr;
    long _len = 0;
    int _offsetX = 0;
    int _offsetY = 0;
    int _plotCount = 0;
    int _rowCount = 0;
    int _skipCount = 0;
    CRGB _bkColor = CRGB::Black;
    CRGB _skipColor = CRGB::Magenta;
} g_gifDecoderState;

GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, 12> g_gifDecoder;

// PatternAnimatedGIF
//
// Draws a cycling animated GIF on the LED matrix.  Use GifDecoder to do the heavy lifting behind the scenes.

class PatternAnimatedGIF : public LEDStripEffect
{
private:

    int       _gifIndex = -1;
    CRGB       _bkColor = BLACK16;
    CRGB     _skipColor = MAGENTA16;

    // GIF decoder callbacks.  These are static because the decoder doesn't allow you to pass any context, so they
    // have to be global.  We use the global g_gifDecoderState to track state.  The GifDecoder code calls back to
    // these callbacks to do the actual work of fetching the bits from teh embedded GIF file and plotting them on
    // the LED matrix.

    static int fileSizeCallback(void) 
    {
        return g_gifDecoderState._len;
    }

    static bool fileSeekCallback(unsigned long position) 
    {
        g_gifDecoderState._seek = position;
        return true;
    }

    static unsigned long filePositionCallback(void) 
    {
        return g_gifDecoderState._seek;
    }

    static int fileReadCallback(void) 
    {
        return pgm_read_byte(g_gifDecoderState._pgif + g_gifDecoderState._seek++);
    }

    static int fileReadBlockCallback(void * buffer, int numberOfBytes) 
    {
        memcpy(buffer, g_gifDecoderState._pgif + g_gifDecoderState._seek, numberOfBytes);
        g_gifDecoderState._seek += numberOfBytes;
        return numberOfBytes; 
    }

    static void screenClearCallback(void) 
    {
        auto& g = *(g_ptrSystem->EffectManager().g());
        g.fillScreen(g.to16bit(g_gifDecoderState._bkColor));
    }

    static void updateScreenCallback(void) 
    {
    }

    static void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) 
    {
        auto& g = *(g_ptrSystem->EffectManager().g(0));
        g.drawPixel(x + g_gifDecoderState._offsetX, y + g_gifDecoderState._offsetY, CRGB(red, green, blue));
        g_gifDecoderState._plotCount++;
        g_gifDecoderState._rowCount = 1;
    }

    // drawLineCallback
    //
    // This is called by the GIF decoder to draw a line of pixels.  

    static void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip) 
    {
        // I don't think this is ever called, but if it is, we need to implement it.  For now, we just
        // call drawPixelCallback for each pixel in the line.  We can remove this if everyone else agrees
        // its safe to do so...

        #if 0 

            debugW("drawLineCallback: y=%d, x=%d, w=%d, skip=%d", y, x, w, skip);

            // Because this is static, we have to get the g object from the EffectManager. So we will always
            // draw to the main surface regardless of the effect's actual drawing context, since at this point
            // we have no idea what effect is actually running when this is executed.

            auto& g = *(g_ptrSystem->EffectManager().g(0));

            uint8_t pixel;
            bool first;

            if (x + w > MATRIX_WIDTH) 
            {
                w = MATRIX_WIDTH - x;
                if (w + x > MATRIX_WIDTH)
                {
                    debugW("drawLineCallback x+w > MATRIX_WIDTH: y=%d, x=%d, w=%d, skip=%d", y, x, w, skip);
                    return;
                }
            }

            if (y >= MATRIX_HEIGHT || x >= MATRIX_WIDTH ) 
            {
                debugW("drawLineCallback invalid args: y=%d, x=%d, w=%d, skip=%d", y, x, w, skip);
                return;
            }


            if (w <= 0) 
            {
                debugW("drawLineCallback w <= 0: y=%d, x=%d, w=%d, skip=%d", y, x, w, skip);
                return;
            }

            int16_t endx = x + w - 1;
            uint16_t buf565[w];
            for (int i = 0; i < w; ) 
            {
                int n = 0;
                while (i < w) {
                    pixel = buf[i++];
                    if (pixel == skip) 
                    {
                        g_gifDecoderState._skipCount++;
                        break;
                    }
                    CRGB color = g.from16Bit(palette[pixel]);
                    if (color == g_gifDecoderState.
                    g.leds[g_gifDecoderState._offsetX + (g_gifDecoderState._offsetY * MATRIX_WIDTH) + n++] = g.from16Bit(pixel);
                }
            }
            g_gifDecoderState._plotCount += w;  //count total pixels (including skipped)
            g_gifDecoderState._rowCount += 1;   //count number of drawLines
        #endif

    }

    // OpenGif
    //
    // Fetches the EmbeddedFile for the given GIF index and sets up the GIF decoder to use it

    bool OpenGif()
    {
        auto gif = AnimatedGIFs.find(_gifIndex);
        if (gif == AnimatedGIFs.end())
        {
            debugW("GIF not found by index: %d", _gifIndex);
            return false;
        }

        uint16_t x, y;
        g_gifDecoder.getSize(&x, &y);

        // Set up the gifDecoderState with all of the context that it will need to decode and
        // draw the GIF, since the static callbacks will have no other context to work with.

        g_gifDecoderState._offsetX = (MATRIX_WIDTH - GIF_WIDTH) / 2;            
        g_gifDecoderState._offsetY = (MATRIX_HEIGHT - GIF_HEIGHT) / 2;
        g_gifDecoderState._bkColor = _bkColor;
        g_gifDecoderState._skipColor = _skipColor;
        g_gifDecoderState._pgif = gif->second.contents;
        g_gifDecoderState._seek = 0;
        g_gifDecoderState._len  = gif->second.length;

        return true;
    }

    size_t DesiredFramesPerSecond() const override
    {
        return 25;
    }

public:

    // 
    PatternAnimatedGIF(const String & friendlyName, int gifIndex, CRGB bkColor = CRGB::Black, CRGB skip = CRGB::Magenta) : 
        LEDStripEffect(EFFECT_MATRIX_ANIMATEDGIF, friendlyName),
        _gifIndex(gifIndex),
        _bkColor(bkColor),
        _skipColor(skip)
    {
    }

    PatternAnimatedGIF(const JsonObjectConst& jsonObject)
        : LEDStripEffect(jsonObject),
          _gifIndex(jsonObject[PTY_GIFINDEX]), 
          _bkColor(jsonObject[PTY_BKCOLOR]),
          _skipColor(jsonObject[PTY_SKIPCOLOR])
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_GIFINDEX]  = _gifIndex;
        jsonDoc[PTY_BKCOLOR]   = _bkColor;
        jsonDoc[PTY_SKIPCOLOR] = _skipColor;

        assert(!jsonDoc.overflowed());
        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    void Start() override
    {
        g()->Clear();
        
        // Set the GIF decoder callbacks to our static functions

        g_gifDecoder.setScreenClearCallback( screenClearCallback );
        g_gifDecoder.setUpdateScreenCallback( updateScreenCallback );
        g_gifDecoder.setDrawPixelCallback( drawPixelCallback );
        g_gifDecoder.setDrawLineCallback( drawLineCallback );

        g_gifDecoder.setFileSeekCallback( fileSeekCallback );
        g_gifDecoder.setFilePositionCallback( filePositionCallback );
        g_gifDecoder.setFileReadCallback( fileReadCallback );
        g_gifDecoder.setFileReadBlockCallback( fileReadBlockCallback );
        g_gifDecoder.setFileSizeCallback( fileSizeCallback );

        // Open the GIF and start decoding
        
        OpenGif();
        if (ERROR_NONE != g_gifDecoder.startDecoding())
            debugW("Failed to start decoding GIF");
    }

    void Draw() override
    {
        if (0 > g_gifDecoder.getFrameNumber())
            return;

        g_gifDecoder.decodeFrame();
    }
};

#endif
