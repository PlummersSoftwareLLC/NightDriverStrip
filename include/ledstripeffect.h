//+--------------------------------------------------------------------------
//
// File:        LEDStripEffect.h
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
// Description:
//
//    Defines the base class for effects that run locally on the chip
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include "effects.h"
#include "jsonserializer.h"
#include "types.h"
#include <memory>

extern bool                               g_bUpdateStarted;
extern DRAM_ATTR std::shared_ptr<GFXBase> g_aptrDevices[NUM_CHANNELS];

#define RETURN_IF_SET(settingName, propertyName, property, value) if (SetIfSelected(settingName, propertyName, property, value)) return true

// LEDStripEffect
//
// Base class for an LED strip effect.  At a minimum they must draw themselves and provide a unique name.

class LEDStripEffect : public IJSONSerializable
{
  protected:

    size_t _cLEDs;
    String _friendlyName;
    int    _effectNumber;
    bool   _enabled = true;
    std::vector<SettingSpec> _settingSpecs;

    std::shared_ptr<GFXBase> _GFX[NUM_CHANNELS];

    template<typename Tv>
    bool SetIfSelected(const String& settingName, const String& propertyName, Tv& property, const Tv& value)
    {
        if (settingName == propertyName)
        {
            property = value;
            return true;
        }

        return false;
    }

  public:

    LEDStripEffect(int effectNumber, const String & strName) :
        _effectNumber(effectNumber)
    {
        if (!strName.isEmpty())
            _friendlyName = strName;
    }

    LEDStripEffect(const JsonObjectConst&  jsonObject)
        : _effectNumber(jsonObject[PTY_EFFECTNR]),
          _friendlyName(jsonObject["fn"].as<String>())
    {
        if (jsonObject.containsKey("es"))
            _enabled = jsonObject["es"].as<int>() == 1;
    }

    virtual ~LEDStripEffect()
    {
    }

    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS])
    {
        debugV("Init %s", _friendlyName.c_str());

        for (int i = 0; i < NUM_CHANNELS; i++)                      // There are up to 8 channel in play per effect and when we
            _GFX[i] = gfx[i];                                       //   start up, we are given copies to their graphics interfaces
                                                                    //   so that we can call them directly later from other calls
        _cLEDs = _GFX[0]->GetLEDCount();

        debugV("Init Effect %s with %d LEDs\n", _friendlyName.c_str(), _cLEDs);
        return true;
    }

    virtual void Start() {}                                         // Optional method called when time to clean/init the effect
    virtual void Draw() = 0;                                        // Your effect must implement these

    inline std::shared_ptr<GFXBase> g() const
    {
        return _GFX[0];
    }

    // mg is a shortcut for MATRIX projects to retrieve a pointer to the specialized LEDMatrixGFX type

    #if USE_MATRIX
      static std::shared_ptr<LEDMatrixGFX> mg()
      {
        return std::static_pointer_cast<LEDMatrixGFX>(g_aptrDevices[0]);
      }
    #endif

    virtual bool CanDisplayVUMeter() const
    {
        return true;
    }

    virtual const String & FriendlyName() const             // User-visible effect name
    {
        return _friendlyName;
    }

    int EffectNumber() const
    {
        return _effectNumber;
    }

    virtual size_t DesiredFramesPerSecond() const           // Desired framerate of the LED drawing
    {
        return 30;
    }

    virtual size_t MaximumEffectTime() const                // For splash screens and similar, a max display time for the effect
    {
        return SIZE_MAX;
    }

    virtual bool HasMaximumEffectTime() const
    {
        return MaximumEffectTime() != SIZE_MAX;
    }

    virtual bool ShouldShowTitle() const                    // True if the effect should show the title overlay
    {
        return true;
    }

    // RequiresDoubleBuffering
    //
    // If a matrix effect requires the state of the last buffer be preserved, then it requires double buffering.
    // If, on the other hand, it renders from scratch every time, starting witha black fill, etc, then it does not,
    // and it can override this method and return false;

    virtual bool RequiresDoubleBuffering() const
    {
        return true;
    }

    // RandomRainbowColor
    //
    // Returns a random color of the rainbow
    // BUGBUG Should likely be in GFXBase, not in LEDStripEffect

    static CRGB RandomRainbowColor()
    {
        static const CRGB colors[] =
            {
                CRGB::Green,
                CRGB::Red,
                CRGB::Blue,
                CRGB::Orange,
                CRGB::Indigo,
                CRGB::Violet
            };
        int randomColorIndex = (int)randomfloat(0, ARRAYSIZE(colors));
        return colors[randomColorIndex];
    }

    // RandomSaturatedColor
    //
    // A random, but fully saturated, color

    static CRGB RandomSaturatedColor()
    {
        CRGB c;
        c.setHSV((uint8_t)randomfloat(0, 255), 255, 255);
        return c;
    }

    // GetBlackBodyHeatColor
    //
    // Given a temp in the 0-1 range, returns a fire-appropriate black body radiator color for it

    static CRGB GetBlackBodyHeatColor(float temp)
    {
        temp = min(1.0f, temp);
        uint8_t temperature = (uint8_t)(255 * temp);
        uint8_t t192 = (uint8_t)((temperature / 255.0f) * 191);

        uint8_t heatramp = (uint8_t)(t192 & 0x3F);
        heatramp <<= 2;

        if (t192 > 0x80)
            return CRGB(255, 255, heatramp);
        else if (t192 > 0x40)
            return CRGB(255, heatramp, 0);
        else
            return CRGB(heatramp, 0, 0);
    }

    // fillSolidOnAllChannels
    //
    // Fill all of the LEDs specified with the color indicated.  Can have arbitrary start, length, and step

    void fillSolidOnAllChannels(CRGB color, int iStart = 0, int numToFill = 0, uint everyN = 1)
    {
        if (!_GFX[0])
            throw std::runtime_error("Graphcis not set up properly");

        if (numToFill == 0)
            numToFill = _cLEDs-iStart;

        if (iStart + numToFill > _cLEDs)
        {
            printf("Boundary Exceeded in FillRainbow");
            return;
        }

        for (int n = 0; n < NUM_CHANNELS; n++)
        {
            for (int i = iStart; i < iStart + numToFill; i+= everyN)
                _GFX[n]->setPixel(i, color);

        }
    }

    // ClearFrameOnAllChannels
    //
    // Clears ALL the channels

    void ClearFrameOnAllChannels()
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
            _GFX[i]->Clear();
    }

    // ColorFraction
    //
    // Returns a fraction of a color; abstracts the fadeToBlack away so that we can later
    // do better color correction as needed

    static CRGB ColorFraction(const CRGB colorIn, float fraction)
    {
        fraction = min(1.0f, fraction);
        fraction = max(0.0f, fraction);
        return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
    }

    // fillRainbowAllChannels
    //
    // Fill all channels with a progressive rainbow, using arbitrary start, length, step, and initial color and hue change rate

    void fillRainbowAllChannels(int iStart, int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t everyNth = 1)
    {
        if (iStart + numToFill > _cLEDs)
        {
            printf("Boundary Exceeded in FillRainbow");
            return;
        }

        CHSV hsv;
        hsv.hue = initialhue;
        hsv.val = 255;
        hsv.sat = 240;
        for (int i = 0; i < numToFill; i+=everyNth)
        {
            CRGB rgb;
            hsv2rgb_rainbow(hsv, rgb);
            setPixelOnAllChannels(iStart + i, rgb);
            hsv.hue += deltahue;
            for (int q = 1; q < everyNth; q++)
                _GFX[q]->setPixel(iStart + i + q, CRGB::Black);
        }
    }

    // fadePixelToBlackOnAllChannelsBy
    //
    // Given a 0-255 fade value, fades all channels by that amount

    void fadePixelToBlackOnAllChannelsBy(int pixel, uint8_t fadeValue) const
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            CRGB crgb = _GFX[i]->getPixel(pixel);
            crgb.fadeToBlackBy(fadeValue);
            _GFX[i]->setPixel(pixel, crgb);
        }
    }

    void fadeAllChannelsToBlackBy(uint8_t fadeValue) const
    {
        for (int i = 0; i < _cLEDs; i++)
            fadePixelToBlackOnAllChannelsBy(i, fadeValue);
    }

    void setAllOnAllChannels(uint8_t r, uint8_t g, uint8_t b) const
    {
        for (int n = 0; n < NUM_CHANNELS; n++)
            for (int i = 0; i < _cLEDs; i++)
                _GFX[n]->setPixel(i, r, g, b);
    }

    // setPixelOnAllChannels
    //
    // Sets the indexed pixel to a given color on all channels

    void setPixelOnAllChannels(int i, CRGB c)
    {
        for (int j = 0; j < NUM_CHANNELS; j++)
            _GFX[j]->setPixel(i, c);
    }

    // setPixelsOnAllChannels
    //
    // Smooth drawing on fractional pixels on all channels in the given color; if merge is specified,
    // color drawing is additive, otherwise replacement

    void setPixelsOnAllChannels(float fPos, float count, CRGB c, bool bMerge = false) const
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
            _GFX[i]->setPixelsF(fPos, count, c, bMerge);
    }

    // SerializeToJSON
    //
    // Serialize this effects paramters to a JSON document

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<128> jsonDoc;

        jsonDoc[PTY_EFFECTNR] = _effectNumber;
        jsonDoc["fn"]         = _friendlyName;
        jsonDoc["es"]         = _enabled ? 1 : 0;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool IsEnabled() const
    {
        return _enabled;
    }

    virtual void SetEnabled(bool enabled)
    {
        _enabled = enabled;
    }

    virtual bool HasSettings() const
    {
        return false;
    }

    virtual const std::vector<SettingSpec>& GetSettingSpecs() const
    {
        return _settingSpecs;
    }

    virtual bool SerializeSettingsToJSON(JsonObject& jsonObject)
    {
        return true;
    }

    virtual bool SetSetting(const String& name, const String& value)
    {
        return false;
    }
};

