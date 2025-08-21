//+--------------------------------------------------------------------------
//
// File:        misceffects.h
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
//    Draws bouncing balls using a kinematics formula
//
// History:     Apr-17-2019         Davepl      Adapted from NightDriver
//
//---------------------------------------------------------------------------

#pragma once

#include <deque>

#if USE_HUB75
#include "TJpg_Decoder.h"
#endif
#include "effects.h"
#include "systemcontainer.h"

// SimpleRainbowTestEffect
//
// Fills the spokes with a rainbow palette, skipping dots as specified

class SimpleRainbowTestEffect : public LEDStripEffect
{
  private:
    uint8_t     _EveryNth;
    uint8_t     _SpeedDivisor;

  public:

    SimpleRainbowTestEffect(uint8_t speedDivisor = 8, uint8_t everyNthPixel = 12)
  : LEDStripEffect(idStripSimpleRainbowTest, "Simple Rainbow"),
          _EveryNth(everyNthPixel),
          _SpeedDivisor(speedDivisor)
    {
        debugV("SimpleRainbowTestEffect constructor");
    }
    static constexpr EffectId kId = idStripSimpleRainbowTest;
    EffectId effectId() const override { return kId; }

    SimpleRainbowTestEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
          _EveryNth(jsonObject[PTY_EVERYNTH]),
          _SpeedDivisor(jsonObject[PTY_SPEEDDIVISOR])
    {
        debugV("SimpleRainbowTestEffect JSON constructor");
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_EVERYNTH] = _EveryNth;
        jsonDoc[PTY_SPEEDDIVISOR] = _SpeedDivisor;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Draw() override
    {
        fillRainbowAllChannels(0, _cLEDs, beatsin16(4, 0, 256), 8, _EveryNth);
        delay(10);
    }
};

// SimpleRainbowTestEffect
//
// Fills the spokes with a rainbow palette, skipping dots as specified

class RainbowTwinkleEffect : public LEDStripEffect
{
  private:
    float _speedDivisor;
    int   _deltaHue;

  public:

    RainbowTwinkleEffect(float speedDivisor = 12.0f, int deltaHue = 14)
  : LEDStripEffect(idStripRainbowTwinkle, "Rainbow Twinkle"),
        _speedDivisor(speedDivisor),
        _deltaHue(deltaHue)
    {
        debugV("RainbowFill constructor");
    }
    static constexpr EffectId kId = idStripRainbowTwinkle;
    EffectId effectId() const override { return kId; }

    RainbowTwinkleEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        _speedDivisor(jsonObject[PTY_SPEEDDIVISOR]),
        _deltaHue(jsonObject[PTY_DELTAHUE])
    {
        debugV("RainbowFill JSON constructor");
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_SPEEDDIVISOR] = _speedDivisor;
        jsonDoc[PTY_DELTAHUE] = _deltaHue;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Draw() override
    {
        static float hue = 0.0f;
        static unsigned long lastms = millis();

        unsigned long msElapsed = millis() - lastms;
        lastms = millis();

        hue += (float) msElapsed / _speedDivisor;
        hue = fmod(hue, 256.0f);
        fillRainbowAllChannels(0, _cLEDs, hue, _deltaHue);

        if (random(0, 1) == 0)
            setPixelOnAllChannels(random(0, _cLEDs), CRGB::White);
        delay(10);
    }
};

// RainbowFillEffect
//
// Fills the spokes with a rainbow palette


class RainbowFillEffect : public LEDStripEffect
{
  public:
    static constexpr EffectId kId = idStripRainbowFill;
    EffectId effectId() const override { return kId; }

    protected:

    float _speedDivisor;
    int   _deltaHue;
    bool  _mirrored;

  public:

    RainbowFillEffect(float speedDivisor = 12.0f, int deltaHue = 14, bool mirrored = false)
     : LEDStripEffect(idStripRainbowFill, "RainbowFill Rainbow"),
        _speedDivisor(speedDivisor),
        _deltaHue(deltaHue),
        _mirrored(mirrored)
    {
        debugV("RainbowFill constructor");
    }

    RainbowFillEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        _speedDivisor(jsonObject[PTY_SPEEDDIVISOR]),
        _deltaHue(jsonObject[PTY_DELTAHUE]),
        _mirrored(jsonObject[PTY_MIRRORED])
    {
        debugV("RainbowFill JSON constructor");
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_SPEEDDIVISOR] = _speedDivisor;
        jsonDoc[PTY_DELTAHUE] = _deltaHue;
        jsonDoc[PTY_MIRRORED] = _mirrored;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Draw() override
    {
        static float hue = 0.0f;
        static unsigned long lastms = millis();

        unsigned long msElapsed = millis() - lastms;
        lastms = millis();

        hue += (float) msElapsed / _speedDivisor;
        hue = fmod(hue, 256.0);
        fillRainbowAllChannels(0, _cLEDs, hue, _deltaHue, 1, _mirrored);
        delay(10);
    }
};

// ColorFillEffect
//
// Fills the pixels with a single color.
// everyNth can be used to light some pixels with specified color, leaving the others unlit.
// Unless a user chooses to ignore the global color, the global color will be used instead when
// DeviceConfig().ApplyGlobalColors() returns true.

class ColorFillEffect : public LEDStripEffect
{
  private:

protected:

    int _everyNth;
    CRGB _color;
    bool _ignoreGlobalColor;

  public:

    ColorFillEffect(const String &name, CRGB color = CRGB(246,200,160), int everyNth = 10, bool ignoreGlobalColor = false)
     : LEDStripEffect(idStripColorFill, name),
        _everyNth(everyNth),
        _color(color),
        _ignoreGlobalColor(ignoreGlobalColor)
    {
        debugV("Color Fill constructor");
    }
    static constexpr EffectId kId = idStripColorFill;
    EffectId effectId() const override { return kId; }

    ColorFillEffect(CRGB color = CRGB(246,200,160), int everyNth = 10, bool ignoreGlobalColor = false)
      : LEDStripEffect(idStripColorFill, "Color Fill"),
        _everyNth(everyNth),
        _color(color),
        _ignoreGlobalColor(ignoreGlobalColor)
    {
        debugV("Color Fill constructor");
    }

    ColorFillEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        _everyNth(jsonObject[PTY_EVERYNTH]),
        _color(jsonObject[PTY_COLOR].as<CRGB>()),
        _ignoreGlobalColor(jsonObject[PTY_IGNOREGLOBALCOLOR])
    {
        debugV("Color Fill JSON constructor");
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_EVERYNTH] = _everyNth;
        jsonDoc[PTY_COLOR] = _color;
        jsonDoc[PTY_IGNOREGLOBALCOLOR] = _ignoreGlobalColor;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Draw() override
    {
        if (_everyNth != 1)
          fillSolidOnAllChannels(CRGB::Black);
        if (!_ignoreGlobalColor && g_ptrSystem->DeviceConfig().ApplyGlobalColors())
          fillSolidOnAllChannels(g_ptrSystem->DeviceConfig().GlobalColor(), 0, NUM_LEDS, _everyNth);
        else
          fillSolidOnAllChannels(_color, 0, NUM_LEDS, _everyNth);
    }
};

#if USE_HUB75

// SplashLogoEffect
//
// Displays the NightDriver logo on the screen

extern const uint8_t logo_start[] asm("_binary_assets_bmp_lowreslogo_jpg_start");
extern const uint8_t logo_end[]   asm("_binary_assets_bmp_lowreslogo_jpg_end");

class SplashLogoEffect : public LEDStripEffect
{
  private:
    EmbeddedFile logo;

  public:

    SplashLogoEffect()
      : LEDStripEffect(idStripSplashLogo, "Mesmerizer"),
        logo(logo_start, logo_end)
    {
        debugV("Splash logo constructor");
    }
    static constexpr EffectId kId = idStripSplashLogo;
    EffectId effectId() const override { return kId; }

    SplashLogoEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        logo(logo_start, logo_end)
    {
        debugV("Splash logo JSON constructor");
    }

    virtual size_t MaximumEffectTime() const override
    {
        return 5.0 * MILLIS_PER_SECOND;
    }

    bool CanDisplayVUMeter() const override
    {
        return false;
    }

    void Draw() override
    {
        fillSolidOnAllChannels(CRGB::Black);
        if (JDR_OK != TJpgDec.drawJpg(0, 0, logo.contents, logo.length))        // Draw the image
            debugW("Could not display logo");
    }
};

#endif // USE_HUB75

// StatusEffect
//
// Effect that shows every 10th LED in a particular color, depending on the state of the system:
//
// Color     Meaning
// -----------------------------------
// Red       No WiFi
// Purple    OTA Update in progress
// Green     WiFi working but no clock yet
// White     Ready!

class StatusEffect : public LEDStripEffect
{
  protected:

    int  _everyNth;
    CRGB _color;

  public:

    StatusEffect(CRGB color = CRGB(255,255,255), int everyNth = 10)     // Warmer: CRGB(246,200,160)
  : LEDStripEffect(idStripStatus, "Status Fill"),
        _everyNth(everyNth),
        _color(color)
    {
        debugV("Status Fill constructor");
    }
    static constexpr EffectId kId = idStripStatus;
    EffectId effectId() const override { return kId; }

    StatusEffect(const JsonObjectConst& jsonObject)     // Warmer: CRGB(246,200,160)
      : LEDStripEffect(jsonObject),
        _everyNth(jsonObject[PTY_EVERYNTH]),
        _color(jsonObject[PTY_COLOR].as<CRGB>())
    {
        debugV("Status Fill JSON constructor");
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_EVERYNTH] = _everyNth;
        jsonDoc[PTY_COLOR] = _color;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Draw() override
    {
        CRGB color = _color;

        if (g_Values.UpdateStarted)
          color = CRGB::Purple;
        else if (!WiFi.isConnected())
          color = CRGB::Red;
        else if (!NTPTimeClient::HasClockBeenSet())
          color = CRGB::Green;

        if (_everyNth != 1)
          fillSolidOnAllChannels(CRGB::Black);
        fillSolidOnAllChannels(color, 0, 0, _everyNth);
    }
};

#if CLASSIC_GE_C9
static constexpr auto TwinkleColors =  to_array(
{
    CRGB(238, 51, 39),      // Red
    CRGB(0, 172, 87),       // Green
    CRGB(250, 164, 25),     // Yellow
    CRGB(0, 131, 203)       // Blue
});
#else
static constexpr auto TwinkleColors =  to_array(
{
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Purple,
    CRGB::Yellow
});
#endif

class TwinkleEffect : public LEDStripEffect
{
  protected:

    int  _countToDraw;
    int  _fadeFactor;
    int  _updateSpeed;

  public:

    TwinkleEffect(int countToDraw = NUM_LEDS / 2, uint8_t fadeFactor = 10, int updateSpeed = 10)
      : LEDStripEffect(idStripTwinkle, "Twinkle"),
        _countToDraw(countToDraw),
        _fadeFactor(fadeFactor),
        _updateSpeed(updateSpeed)
    {
    }
    static constexpr EffectId kId = idStripTwinkle;
    EffectId effectId() const override { return kId; }

    TwinkleEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        _countToDraw(jsonObject["ctd"]),
        _fadeFactor(jsonObject[PTY_FADE]),
        _updateSpeed(jsonObject[PTY_SPEED])
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["ctd"] = _countToDraw;
        jsonDoc[PTY_FADE] = _fadeFactor;
        jsonDoc[PTY_SPEED] = _updateSpeed;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    const int Count = 99;
    int buffer[99] = { 0 };

    std::deque<size_t> litPixels;

    void Draw() override
    {
        EVERY_N_MILLISECONDS(_updateSpeed)
        {
            while(litPixels.size() > _countToDraw)
            {
                size_t i = litPixels.back();
                litPixels.pop_back();
                setPixelOnAllChannels(i, CRGB::Black);
            }

            // Pick a random pixel and put it in the TOP slot
            for (int iLoop = 0; iLoop < 2; iLoop++)
            {
              int iNew = -1;
              for (int iPass = 0; iPass < NUM_LEDS * 20; iPass++)
              {
                  size_t i = random(0, NUM_LEDS);
                  if (_GFX[0]->getPixel(i) != CRGB::Black)
                      continue;
                  if (litPixels.end() != find(litPixels.begin(), litPixels.end(), i))
                      continue;
                  iNew = i;
                  break;
              }
              if (iNew == -1)             // No empty slot could be found!
              {
                  litPixels.clear();
                  setAllOnAllChannels(0,0,0);
                  return;
              }
              assert(litPixels.end() == find(litPixels.begin(), litPixels.end(), iNew));
              setPixelOnAllChannels(iNew, TwinkleColors[random(0, std::size(TwinkleColors))]);
              litPixels.push_front(iNew);
            }
        }

        EVERY_N_MILLISECONDS(20)
        {
            fadeAllChannelsToBlackBy(_fadeFactor);
        }
    }
};

// SilonEffect
//
// A Battlestar Galactica inspired effect that moves red and green bars back and forth

class SilonEffect : public LEDStripEffect
{
  public:

  SilonEffect() : LEDStripEffect(idMatrixSilon, "SilonEffect")
    {
    }
    static constexpr EffectId kId = idMatrixSilon;
    EffectId effectId() const override { return kId; }

    SilonEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject)
    {
    }

    int _offset = 0;
    int _direction = 1;

    virtual size_t DesiredFramesPerSecond() const
    {
        return 20;
    }

    virtual void Draw() override
    {
        _offset += _direction;
        if (_offset >= MATRIX_WIDTH)
        {
            _offset = MATRIX_WIDTH - 1;
            _direction = -1;
        }
        if (_offset <= 0)
        {
            _offset = 0;
            _direction = 1;
        }
        fadeAllChannelsToBlackBy(75);

        for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            setPixelOnAllChannels(_offset, y, CRGB::Red);
            setPixelOnAllChannels(MATRIX_WIDTH - 1 - _offset, y, CRGB::Green);
        }
    }
};

// PDPGridEffect
//
// A Display for the front of the PDP-11/34

class PDPGridEffect : public LEDStripEffect
{
  public:

  PDPGridEffect() : LEDStripEffect(idMatrixPDPGrid, "PDPGridEffect")
    {
    }
    static constexpr EffectId kId = idMatrixPDPGrid;
    EffectId effectId() const override { return kId; }

    PDPGridEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject)
    {
    }

    int _offset = 0;
    int _direction = 1;

    virtual size_t DesiredFramesPerSecond() const
    {
        return 5;
    }

    bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    virtual void Start() override
    {
        g()->Clear();
    }

    virtual void Draw() override
    {
        fadeAllChannelsToBlackBy(60);
        g()->MoveY(1);
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            // Pick a color, CRGB::Red 90% of the time, CRGB::Green 10% of the time
            CRGB color = random(0, 100) > 20 ? CRGB::Black : CRGB(random(0, 100) < 90) ? CRGB::Red : CRGB::Orange;
            setPixelOnAllChannels(x, MATRIX_HEIGHT-1, color);
        }
    }
};

// PDPCMXEffect
//
// Connection Machine 5 LED simulation for the PDP-11/34 CMX display

class PDPCMXEffect : public LEDStripEffect
{
  private:
    static constexpr int GROUP_HEIGHT = 4; // Height of each logical group
    static constexpr float LED_PROBABILITY = 0.30f; // 30% chance of LED being on

  void scrollGroup(int groupStartY, bool scrollLeft)
    {
        // Scroll existing LEDs in the group
        for (int y = groupStartY; y < groupStartY + GROUP_HEIGHT && y < MATRIX_HEIGHT; y++)
        {
            if (scrollLeft)
            {
                // Scroll left: move all pixels one position left
                for (int x = 0; x < MATRIX_WIDTH - 1; x++)
                {
                    CRGB color = _GFX[0]->getPixel(x + 1, y);
                    setPixelOnAllChannels(x, y, color);
                }
                // Clear the rightmost pixel (will be populated with new random data)
                setPixelOnAllChannels(MATRIX_WIDTH - 1, y, CRGB::Black);
            }
            else
            {
                // Scroll right: move all pixels one position right
                for (int x = MATRIX_WIDTH - 1; x > 0; x--)
                {
                    CRGB color = _GFX[0]->getPixel(x - 1, y);
                    setPixelOnAllChannels(x, y, color);
                }
                // Clear the leftmost pixel (will be populated with new random data)
                setPixelOnAllChannels(0, y, CRGB::Black);
            }
        }

    // Add new random LEDs on the appropriate edge
    // Color by band parity: even bands = red, odd bands = amber
        const int groupIndex = groupStartY / GROUP_HEIGHT;
        const bool isEvenGroup = (groupIndex % 2) == 0;
        const CRGB bandColor = isEvenGroup ? CRGB::Red : CRGB::Orange;

        for (int y = groupStartY; y < groupStartY + GROUP_HEIGHT && y < MATRIX_HEIGHT; y++)
        {
            if (random(100) < (LED_PROBABILITY * 100))
            {
              if (scrollLeft)
                setPixelOnAllChannels(MATRIX_WIDTH - 1, y, bandColor);
              else
                setPixelOnAllChannels(0, y, bandColor);
            }
        }
    }

  public:

    static constexpr EffectId kId = idMatrixPDPCMX;
    EffectId effectId() const override { return kId; }

    PDPCMXEffect() : LEDStripEffect(kId, "PDPCMXEffect")
    {
    }

    PDPCMXEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const
    {
        return 10; // Moderate speed for scrolling effect
    }

    virtual bool CanDisplayVUMeter() const override
    {
        return false;
    }

    virtual void Start() override
    {
        g()->Clear();
    }

    virtual void Draw() override
    {
        // Process each logical group
        int numGroups = (MATRIX_HEIGHT + GROUP_HEIGHT - 1) / GROUP_HEIGHT; // Ceiling division

        fadeAllChannelsToBlackBy(3);
        for (int group = 0; group < numGroups; group++)
        {
            int groupStartY = group * GROUP_HEIGHT;
            bool scrollLeft = (group % 2 == 0); // Alternate direction: even groups scroll left, odd scroll right

            scrollGroup(groupStartY, scrollLeft);
        }
    }
};

#if HEXAGON
////////////////////////////////////////////////
// Hexagon Effects
////////////////////////////////////////////////

class OuterHexRingEffect : public LEDStripEffect
{
  public:

  OuterHexRingEffect() : LEDStripEffect(idHexagonOuterRing, "OuterRingHexEffect")
    {
    }
    static constexpr EffectId kId = idHexagonOuterRing;
    EffectId effectId() const override { return kId; }

    OuterHexRingEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject)
    {
    }

    virtual void Draw() override
    {
        static int colorOffset = HUE_BLUE;
        static int indent = 0;

        EVERY_N_MILLIS(20)
          colorOffset += 4;

        EVERY_N_MILLIS(100)
          indent = (indent + 1) % 9;

        fadeAllChannelsToBlackBy(75);

        CRGB color = ColorFromPalette(RainbowColors_p, indent*32 + colorOffset);
        hg()->fillHexRing(indent, color);
    }
};
#endif // HEXAGON

