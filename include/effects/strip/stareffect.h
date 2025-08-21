//+--------------------------------------------------------------------------
//
// File:        StarEffect.h
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
//   An effect that twinkles stars in and out.  Adapted from my NightDriver
//   code.
//
// History:     Nov-6-2018         Davepl      Commented
//
//---------------------------------------------------------------------------

#pragma once

#include <deque>

#include "particles.h"

const int cMaxNewStarsPerFrame = 144;
const int cMaxStars = 500;
const int starWidth = 1;


class Star : public MovingFadingPaletteObject, public ObjectSize
{
  public:

    static int GetStarTypeNumber()
    {
        return idStar;
    }

    virtual float GetStarSize()
    {
        return _objectSize;
    }

    Star(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, float maxSpeed = 1.0, float starSize = 1.0)
        : MovingFadingPaletteObject(palette, blendType, maxSpeed),
          ObjectSize(starSize)
    {
    }
};

class RandomPaletteColorStar : public MovingFadingPaletteObject, public ObjectSize
{
  public:

    static int GetStarTypeNumber()
    {
    return idStarRandomPaletteColor;
    }

    virtual float GetStarSize()
    {
        return _objectSize;
    }

    RandomPaletteColorStar(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, float maxSpeed = 1.0, float starSize = 1.0)
        : MovingFadingPaletteObject(palette, blendType, maxSpeed, random(16)*16),
          ObjectSize(starSize)
    {
    }
};

class LongLifeSparkleStar : public MovingFadingPaletteObject, public ObjectSize
{
  public:

    static int GetStarTypeNumber()
    {
        return idStarLongLifeSparkle;
    }

    virtual float GetStarSize()
    {
        return _objectSize;
    }

    LongLifeSparkleStar(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, float maxSpeed = 1.0, float starSize = 1.0)
        : MovingFadingPaletteObject(palette, blendType, maxSpeed),
          ObjectSize(starSize)
    {
    }

    float PreignitionTime() const override         { return .25f;  }
    float IgnitionTime()    const override         { return 5.0f;  }
    float HoldTime()        const override         { return 0.00f; }
    float FadeTime()        const override         { return 0.0f;  }
};

class ColorStar : public MovingFadingColoredObject, public ObjectSize
{
  public:

    static int GetStarTypeNumber()
    {
        return idStarColor;
    }

    virtual float GetStarSize()
    {
        return _objectSize;
    }

    ColorStar(CRGB color, float maxSpeed = 1.0, float starSize = 1.0)
        : MovingFadingColoredObject(color, maxSpeed),
          ObjectSize(starSize)
    {
    }
};

class QuietStar : public RandomPaletteColorStar
{
  public:

    static int GetStarTypeNumber()
    {
        return idStarQuiet;
    }

    QuietStar(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, float maxSpeed = 10.0, float starSize = 1)
      : RandomPaletteColorStar(palette, blendType, maxSpeed, starSize)
    {}

    float PreignitionTime() const override { return 1.0f; }
    float IgnitionTime()    const override { return 0.00f; }
    float HoldTime()        const override { return 0.00f; }
    float FadeTime()        const override { return 2.0f;  }
    virtual float StarSize()        const { return 1;     }
};

#if ENABLE_AUDIO
class MusicStar : public Star
{
  public:

    MusicStar(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, float maxSpeed = 2.0, float starSize = 1)
      : Star(palette, blendType, maxSpeed, starSize)
    {
    }

    static int GetStarTypeNumber()
    {
        return idStarMusic;
    }

    virtual float PreignitionTime() const      { return 0.0f;  }
    virtual float IgnitionTime()    const      { return 0.00f; }
    virtual float HoldTime()        const      { return 0.00f; }
    virtual float FadeTime()        const      { return 0.5f; }

};

class MusicPulseStar : public Star
{
  public:

    MusicPulseStar(const CRGBPalette16 & palette, TBlendType blendType = LINEARBLEND, float maxSpeed = 0.0, float size = 0.0)
      : Star(palette, blendType, maxSpeed, size)
    {

    }

    virtual ~MusicPulseStar() {}

    static int GetStarTypeNumber()
    {
        return idStarMusicPulse;
    }

    virtual float PreignitionTime() const { return 0.00f;  }
    virtual float IgnitionTime()    const { return 0.00f; }
    virtual float HoldTime()        const { return 1.00f;  }
    virtual float FadeTime()        const { return 2.00f; }
    virtual float GetStarSize()    const { return 1 + _objectSize * g_Analyzer.VURatio(); }
};

#endif

class BubblyStar : public Star
{
  protected:

    int         _hue;

  public:

    BubblyStar(const CRGBPalette16 & palette, TBlendType blendType = LINEARBLEND, float maxSpeed = 2.0, float starSize = 12)
      : Star(palette, blendType, maxSpeed, starSize)
    {
        static float lastHue = 0;
        _hue        = lastHue;
        lastHue += 16;
        lastHue = fmod(lastHue, 256);
    }

    static int GetStarTypeNumber()
    {
        return idStarBubbly;
    }

    float GetStarSize() override
    {
        float x = Age()/TotalLifetime();
        float ratio1 = -1 * (2*(x-.5)) * (2*(x-.5)) + 1;
        float width = ratio1 * _objectSize;
        return width;
    }

    ~BubblyStar() override
    {}

    float PreignitionTime() const override  { return 0.00f; }
    float IgnitionTime()    const override  { return 0.05f; }
    float HoldTime()        const override  { return 0.25f; }
    float FadeTime()        const override  { return 0.50f; }
};

class FlashStar : public Star
{
  public:

    static int GetStarTypeNumber()
    {
        return idStarFlash;
    }

    float PreignitionTime() const override  { return 0.00f; }
    float IgnitionTime()    const override  { return 0.10f; }
    float HoldTime()        const override  { return 0.10f; }
    float FadeTime()        const override  { return 0.05f; }
};

class ColorCycleStar : public Star
{
  protected:

    int         _brightness;

  public:

    ColorCycleStar(const CRGBPalette16 & palette, TBlendType blendType = LINEARBLEND, float maxSpeed = 2.0, int speedDivisor = 1)
      : Star(palette, blendType, maxSpeed)
    {
        _brightness = random_range(128,255);
    }

    static int GetStarTypeNumber()
    {
        return idStarColorCycle;
    }

    virtual CRGB Render(TBlendType blend)
    {
        CRGB c = ColorFromPalette(_palette, millis() / 2048.0f, _brightness, blend);
        fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
        return c;
    }

    ~ColorCycleStar() override
    {}

    float PreignitionTime() const override { return 2.0f; }
    float IgnitionTime()    const override { return 0.0f;}
    float HoldTime()        const override { return 2.00f; }
    float FadeTime()        const override { return 0.5f;  }
    virtual float StarSize()        const { return 1;     }
};

class MultiColorStar : public Star
{
  protected:

    uint8_t         _brightness;
    uint8_t         _hue;

  public:

    MultiColorStar(const CRGBPalette16 & palette, TBlendType blendType = LINEARBLEND, float maxSpeed = 2.0, int speedDivisor = 1)
      : Star(palette, blendType, maxSpeed)
    {
        _brightness = random_range(128,255);
        _hue        = random_range(0, 255);
    }

    virtual CRGB Render(TBlendType blend)
    {
        CRGB c = ColorFromPalette(_palette, _hue, _brightness, blend);
        fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
        return c;
    }

    ~MultiColorStar() override
    {}

    static int GetStarTypeNumber()
    {
    return idStarMultiColor;
    }

    float PreignitionTime() const override { return 2.0f; }
    float IgnitionTime()    const override { return 0.0f; }
    float HoldTime()        const override { return 2.00f;}
    float FadeTime()        const override { return 0.5f; }
    virtual float StarSize()        const { return 1;    }
};

class ChristmasLightStar : public Star
{
  public:

    ChristmasLightStar(const CRGBPalette16 & palette, TBlendType blendType, float maxSpeed = 0.0)
      : Star(palette, blendType, maxSpeed, 1.0)

    {
        int iColor = random_range(0,255);
        _colorIndex = iColor;
    }

    ~ChristmasLightStar() override
    {}

    static int GetStarTypeNumber()
    {
        return idStarChristmas;
    }

    float PreignitionTime() const override { return 0.20f; }
    float IgnitionTime()    const override { return 0.00f; }
    float HoldTime()        const override { return 6.0f;  }
    float FadeTime()        const override { return 1.25f; }
    virtual float StarSize()        const { return 0.00f; }
};

// Hot white stars that cool down through white, yellow, red

class HotWhiteStar : public Star
{
  public:

    HotWhiteStar(const CRGBPalette16 & palette, TBlendType blendType = LINEARBLEND, float maxSpeed = 0.0, float size = 0.0)
      : Star(palette, blendType, maxSpeed, size)
    {
    }

    ~HotWhiteStar() override
    {}

    static int GetStarTypeNumber()
    {
        return idStarHotWhite;
    }

    float PreignitionTime() const override { return 0.00f;  }
    float IgnitionTime()    const override { return 0.20f;  }
    float HoldTime()        const override { return 0.00f;  }
    float FadeTime()        const override { return 2.00f;  }

    virtual CRGB RenderColor(TBlendType blend)
    {
        if (Age() < IgnitionTime() + HoldTime())
            return CRGB::White;
        CRGB c = ColorFromPalette(_palette, 130*(1.0f-FadeoutAmount()), 256*(1.0f-FadeoutAmount()), blend);
        return c;
    }
};



/*
template <typename ObjectType> class BeatStarterEffect : public BeatEffectBase
{
  protected:

    uint16_t                        _maxSize;

  public:

    BeatStarterEffect<ObjectType>(uint16_t )

    virtual void HandleBeat(bool bMajor, float elapsed, float span)
    {
        ObjectType newstar(_palette, _blendType, _maxSpeed * _musicFactor, _starSize);
        // This always starts stars on even pixel boundaries so they look like the desired width if not moving
        newstar._iPos = (int) random_range(0, _cLEDs-1-starWidth);
        _allParticles.push_back(newstar);

    }

    virtual void Draw() override
    {
        BeatEffectBase::Draw();
    }
};

*/

// StarryNightEffect template
//
// Generates up to

template <typename StarType> class StarryNightEffect : public EffectWithId<idStripStarryNight>
{
  protected:

    std::deque<StarType>         _allParticles;
    const CRGBPalette16         _palette;
    float                        _newStarProbability;
    float                        _starSize;
    const TBlendType             _blendType;
    float                       _maxSpeed;
    float                       _blurFactor;
    float                       _musicFactor;
    CRGB                         _skyColor;

  public:

    StarryNightEffect(const String & strName,
                      const CRGBPalette16& palette,
                      float probability = 1.0,
                      float starSize = 1.0,
                      TBlendType blendType = LINEARBLEND,
                      float maxSpeed = 100.0,
                      float blurFactor = 0.0,
                      float musicFactor = 1.0,
                      CRGB skyColor = CRGB::Black)
      : EffectWithId<idStripStarryNight>(strName),
        _palette(palette),
        _newStarProbability(probability),
        _starSize(starSize),
        _blendType(blendType),
        _maxSpeed(maxSpeed),
        _blurFactor(blurFactor),
        _musicFactor(musicFactor),
        _skyColor(skyColor)
    {
    }

    StarryNightEffect<StarType>(const JsonObjectConst& jsonObject)
      : EffectWithId<idStripStarryNight>(jsonObject),
        _palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>()),
        _newStarProbability(jsonObject["spb"]),
        _starSize(jsonObject[PTY_SIZE]),
        _blendType(static_cast<TBlendType>(jsonObject[PTY_BLEND])),
        _maxSpeed(jsonObject[PTY_MAXSPEED]),
        _blurFactor(jsonObject[PTY_BLUR]),
        _musicFactor(jsonObject["msf"]),
        _skyColor(jsonObject[PTY_COLOR].as<CRGB>())
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _palette;
        jsonDoc[PTY_STARTYPENR] = StarType::GetStarTypeNumber();
        jsonDoc["spb"] = _newStarProbability;
        jsonDoc[PTY_SIZE] = _starSize;
        jsonDoc[PTY_BLEND] = to_value(_blendType);
        jsonDoc[PTY_MAXSPEED] = _maxSpeed;
        jsonDoc[PTY_BLUR] = _blurFactor;
        jsonDoc["msf"] = _musicFactor;
        jsonDoc[PTY_COLOR] = _skyColor;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    virtual float StarSize()
    {
        return _starSize;
    }

    virtual void Clear()
    {
        LEDStripEffect::setAllOnAllChannels(_skyColor.r, _skyColor.g, _skyColor.b);
    }


    virtual void CreateStars()
    {
    #if ENABLE_AUDIO

        for (int i = 0; i < cMaxNewStarsPerFrame; i++)
        {
            double prob = _newStarProbability;

            prob = (prob / 100) + (g_Analyzer.VURatio() - 1.0) * _musicFactor;

            constexpr auto kProbabilitySpan = 1.0;

            if (g_Analyzer.VU() > 0)
            {
                if (random_range(0.0, kProbabilitySpan) < g_Values.AppTime.LastFrameTime() * prob)
                {
                    StarType newstar(_palette, _blendType, _maxSpeed * _musicFactor, _starSize);
                    // This always starts stars on even pixel boundaries so they look like the desired width if not moving
                    newstar._iPos = (int) random_range(0U, _cLEDs-1-starWidth);
                    _allParticles.push_back(newstar);
                }
            }
        }
    #endif
    }

    virtual void Update()
    {
        // Any particles that have lived their lifespan can be removed.  They should be found at the back of the array
        while (_allParticles.size() > 0 && (_allParticles.front().Age() >= _allParticles.front().TotalLifetime()))
            _allParticles.pop_front();

        // If we've created too many stars, prune the newest first, before they get painted
        while (_allParticles.size() > cMaxStars)
            _allParticles.pop_back();
    }

    void Draw() override
    {
        CreateStars();
        Update();

        if (_blurFactor == 0)
        {
            Clear();
        }
        else
        {
            g()->blurRows(g()->leds, MATRIX_WIDTH, MATRIX_HEIGHT, 0, _blurFactor * 255);
            fadeAllChannelsToBlackBy(55 * (2.0 - g_Analyzer.VURatioFade()));
        }

        for(auto i = _allParticles.begin(); i != _allParticles.end(); i++)
        {
            i->UpdatePosition();
            float fPos = i->_iPos;
            CRGB c = i->ObjectColor();
            setPixelsFOnAllChannels(fPos - i->_objectSize / 2.0, i->_objectSize, c, true);
        }
    }
};

template <typename StarType> class BlurStarEffect : public StarryNightEffect<StarType>
{
  public:

    BlurStarEffect(const CRGBPalette16 & palette, float probability = 0.2, size_t starSize = 1, TBlendType blendType = LINEARBLEND, float maxSpeed = 20.0)
        : StarryNightEffect<StarType>(palette, probability, starSize, blendType, maxSpeed)
    {
    }

    BlurStarEffect(const JsonObjectConst& jsonObject)
        : StarryNightEffect<StarType>(jsonObject)
    {
    }

    virtual void Clear()
    {
        StarryNightEffect<StarType>::setAll(32,0,0);
    }
};

// TwinkleStarEffect
//
// Twinkles random colored dots on and off

class TwinkleStarEffect : public EffectWithId<idStripTwinkleStar>
{
  private:

    #define NUM_TWINKLES 100
    int buffer[NUM_TWINKLES];

  public:

    TwinkleStarEffect() : EffectWithId<idStripTwinkleStar>("Twinkle Star") {}
    TwinkleStarEffect(const JsonObjectConst& jsonObject) : EffectWithId<idStripTwinkleStar>(jsonObject) {}

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        LEDStripEffect::Init(gfx);
        for (int i = 0; i < NUM_TWINKLES; i++)
            buffer[i] = -1;
        return true;
    }

    void Draw() override
    {
        // Rotate the buffer

        for (int i = 0; i < NUM_TWINKLES - 1; i++)
            buffer[i] = buffer[i + 1];

        // If we had a valid pixel in slot 0, we can blank it now

        if (buffer[0] >= 0)
            setPixelsOnAllChannels(buffer[0], 0, 0, 0);

        // Pick a random pixel and put it in the TOP slot

        int iNew = (int) random_range(0U, _cLEDs);
        setPixelOnAllChannels(iNew, RandomRainbowColor());
        buffer[NUM_TWINKLES - 1] = iNew;
    }
};
