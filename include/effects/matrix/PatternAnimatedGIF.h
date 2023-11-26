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
extern const uint8_t pacman_start[]          asm("_binary_assets_gif_pacman_gif_start");
extern const uint8_t pacman_end[]            asm("_binary_assets_gif_pacman_gif_end");
// AnimatedGIFs
//
// Our set of embedded GIFs.  Currently assumed to be 32x32 in size, default FPS.

enum
{
    INVALID     = 0,
    Atomic      = 1,
    ColorSphere = 2,
    Pacman      = 3
} GIFIdentifier;

// GIFInfo
// 
// Extended "EmbeddedFile" that also tracks the width and height of the GIF

struct GIFInfo : public EmbeddedFile
{
    uint16_t        width;
    uint16_t        height;
    GIFInfo(const uint8_t start[], const uint8_t end[], uint16_t width, uint16_t height) : EmbeddedFile(start, end), width(width), height(height) 
    {}
};

static std::map<int, GIFInfo, std::less<int>, psram_allocator<std::pair<int, GIFInfo>>> AnimatedGIFs =
{
    { Pacman, GIFInfo(pacman_start, pacman_end, 64, 12) },
    { Atomic, GIFInfo(atomic_start, atomic_end, 32, 32) },
    { ColorSphere, GIFInfo(colorsphere_start, colorsphere_end, 32, 32) },
};

// The decoder needs us to track some state, but there's only one instance of the decoder, and 
// we can't pass it a pointer to our state because the callback doesn't allow you to pass any
// context, and you can't use a lambda that captures the this pointer because that can't be
// converted to a callback function pointer.  So we have to use a global. 

struct 
{
    long            _seek      = 0;
    const uint8_t * _pgif      = nullptr;
    long            _len       = 0;
    int             _offsetX   = 0;
    int             _offsetY   = 0;
    CRGB            _bkColor   = CRGB::Black;
    CRGB            _skipColor = CRGB::Magenta;
} 
g_gifDecoderState;

// We dynamically allocate the GIF decoder because it's pretty big and we don't want to waste the base
// ram on it.  This way it, and the GIFs it decodes, can live in PSRAM.

std::unique_ptr<GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, 12>> g_ptrGIFDecoder = make_unique_psram<GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, 12>>();

// PatternAnimatedGIF
//
// Draws a cycling animated GIF on the LED matrix.  Use GifDecoder to do the heavy lifting behind the scenes.

class PatternAnimatedGIF : public LEDStripEffect
{
private:
    
    int  _gifIndex  = -1;
    CRGB _bkColor   = BLACK16;
    CRGB _skipColor = MAGENTA16;

    // GIF decoder callbacks.  These are static because the decoder doesn't allow you to pass any context, so they
    // have to be global.  We use the global g_gifDecoderState to track state.  The GifDecoder code calls back to
    // these callbacks to do the actual work of fetching the bits from teh embedded GIF file and plotting them on
    // the LED matrix.

    static int fileSizeCallback(void) 
    {
        return g_gifDecoderState._len;
    }

    // Seek to the given position in the GIF file.  The GIF decoder will call this to seek to a position in the GIF file.

    static bool fileSeekCallback(unsigned long position) 
    {
        g_gifDecoderState._seek = position;
        return true;
    }

    // Return the current position in the GIF file.  The GIF decoder will call this to get the current position.

    static unsigned long filePositionCallback(void) 
    {
        return g_gifDecoderState._seek;
    }

    // Read a byte from the GIF file.  The GIF decoder will call this to read the GIF data.

    static int fileReadCallback(void) 
    {
        return pgm_read_byte(g_gifDecoderState._pgif + g_gifDecoderState._seek++);
    }

    // Read N bytes from the GIF file into the buffer.  The GIF decoder will call this to read the GIF data.

    static int fileReadBlockCallback(void * buffer, int numberOfBytes) 
    {
        memcpy(buffer, g_gifDecoderState._pgif + g_gifDecoderState._seek, numberOfBytes);
        g_gifDecoderState._seek += numberOfBytes;
        return numberOfBytes; 
    }

    // screenClearCallback - clears the screen with the color given to the constructor

    static void screenClearCallback(void) 
    {
        auto& g = *(g_ptrSystem->EffectManager().g());
        g.fillScreen(g.to16bit(g_gifDecoderState._bkColor));
    }

    // We decide when to update the screen, so this is a no-op

    static void updateScreenCallback(void) 
    {
    }

    // drawPixelCallback
    // 
    // This is called by the GIF decoder to draw a pixel.  We use the offset to center the GIF on the LED matrix.

    static void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) 
    {
        auto& g = *(g_ptrSystem->EffectManager().g(0));
        if (false == g.isValidPixel(x  + g_gifDecoderState._offsetX, y + g_gifDecoderState._offsetY))
        {
            debugW("drawPixelCallbackInvalid pixel: %d, %d", x + g_gifDecoderState._offsetX, y + g_gifDecoderState._offsetY);
            return;
        }

        g.leds[XY(x + g_gifDecoderState._offsetX, y + g_gifDecoderState._offsetY)] = CRGB(red, green, blue);
    }

    // drawLineCallback
    //
    // This is called by the GIF decoder to draw a line of pixels.  

    static void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip) 
    {
        // I don't think this is ever called, but if it is, we may need to implement it.  For now, it seems they just
        // call drawPixelCallback for each pixel in the image.  
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
        g_ptrGIFDecoder->getSize(&x, &y);

        // Set up the gifDecoderState with all of the context that it will need to decode and
        // draw the GIF, since the static callbacks will have no other context to work with.

        g_gifDecoderState._offsetX   = (MATRIX_WIDTH - gif->second.width) / 2;            
        g_gifDecoderState._offsetY   = (MATRIX_HEIGHT - gif->second.height) / 2;
        g_gifDecoderState._bkColor   = _bkColor;
        g_gifDecoderState._skipColor = _skipColor;
        g_gifDecoderState._pgif      = gif->second.contents;
        g_gifDecoderState._seek      = 0;
        g_gifDecoderState._len       = gif->second.length;

        return true;
    }

    size_t DesiredFramesPerSecond() const override
    {
        return 24;
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

        g_ptrGIFDecoder->setScreenClearCallback( screenClearCallback );
        g_ptrGIFDecoder->setUpdateScreenCallback( updateScreenCallback );
        g_ptrGIFDecoder->setDrawPixelCallback( drawPixelCallback );
        g_ptrGIFDecoder->setDrawLineCallback( drawLineCallback );

        g_ptrGIFDecoder->setFileSeekCallback( fileSeekCallback );
        g_ptrGIFDecoder->setFilePositionCallback( filePositionCallback );
        g_ptrGIFDecoder->setFileReadCallback( fileReadCallback );
        g_ptrGIFDecoder->setFileReadBlockCallback( fileReadBlockCallback );
        g_ptrGIFDecoder->setFileSizeCallback( fileSizeCallback );

        // Open the GIF and start decoding
        
        OpenGif();
        if (ERROR_NONE != g_ptrGIFDecoder->startDecoding())
            debugW("Failed to start decoding GIF");
    }

    void Draw() override
    {
        g_ptrGIFDecoder->decodeFrame();
    }
};

#endif
