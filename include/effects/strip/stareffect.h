#pragma once
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


#include <algorithm>
#include <deque>
#include <type_traits>

#include "particles.h"
#include "random_utils.h"

const int cMaxNewStarsPerFrame = 144;
const int cMaxStars = 500;
const int starWidth = 1;


class Star : public MovingFadingPaletteObject, public ObjectSize
{
  public:

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


template <typename StarType, typename TEffect>
class StarEffectBase : public EffectWithId<StarEffectBase<StarType, TEffect>>
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
    uint32_t                    _lastMusicBeatSequence = 0;
    uint32_t                    _lastMusicNearBeatSequence = 0;
    bool                         _useBeatColorCoding = false;
    std::deque<int16_t>          _pendingMusicStarColors;

    // Effect name is referenced by the constructors regardless of ENABLE_AUDIO so it must
    // remain visible on audio-less builds (demo). The indices below are only used by the
    // audio-driven beat path and stay behind the guard.
    static constexpr const char* kRgbMusicBlendStarsName = "RGB Music Blend Stars";

    #if ENABLE_AUDIO
    static constexpr int16_t kMusicStarRandomIndex = -1;
    static constexpr uint8_t kMusicStarRedIndex = 0;
    static constexpr uint8_t kMusicStarGreenIndex = 16;
    static constexpr uint8_t kMusicStarBlueIndex = 32;

    void QueueMusicStarsForBeat(const BeatInfo& beat, bool acceptedBeat)
    {
        if constexpr (std::is_same_v<StarType, MusicStar>)
        {
            const float confidence = std::clamp(beat.confidence, 0.0f, 1.5f);
            const float strength = std::clamp(beat.strength, 0.0f, 2.5f);
            const float scale = acceptedBeat ? 1.0f : 0.45f;
            const bool strongBeat = acceptedBeat && strength >= 2.40f && confidence >= 1.20f;
            const float majorBonus = strongBeat ? 8.0f : 0.0f;
            const size_t minStars = acceptedBeat ? 5U : 1U;
            const size_t maxStars = acceptedBeat ? 36U : 14U;
            const int16_t colorIndex = _useBeatColorCoding
                ? (acceptedBeat
                    ? (strongBeat ? kMusicStarRedIndex : kMusicStarGreenIndex)
                    : kMusicStarBlueIndex)
                : kMusicStarRandomIndex;
            const size_t stars = std::clamp<size_t>(
                static_cast<size_t>((acceptedBeat ? 4.0f : 1.0f)
                    + confidence * 8.0f * scale
                    + strength * 8.0f * scale
                    + majorBonus),
                minStars,
                maxStars);

            for (size_t i = 0; i < stars && _pendingMusicStarColors.size() < cMaxNewStarsPerFrame; ++i)
                _pendingMusicStarColors.push_back(colorIndex);
        }
    }
    #endif

  public:

    StarEffectBase(const String & strName,
                   const CRGBPalette16& palette,
                   float probability = 1.0,
                   float starSize = 1.0,
                   TBlendType blendType = LINEARBLEND,
                   float maxSpeed = 100.0,
                   float blurFactor = 0.0,
                   float musicFactor = 1.0,
                   CRGB skyColor = CRGB::Black)
      : EffectWithId<StarEffectBase<StarType, TEffect>>(strName),
        _palette(palette),
        _newStarProbability(probability),
        _starSize(starSize),
        _blendType(blendType),
        _maxSpeed(maxSpeed),
        _blurFactor(blurFactor),
        _musicFactor(musicFactor),
        _skyColor(skyColor),
        _useBeatColorCoding(strName == kRgbMusicBlendStarsName)
    {
    }

    StarEffectBase(const JsonObjectConst& jsonObject)
      : EffectWithId<StarEffectBase<StarType, TEffect>>(jsonObject),
        _palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>()),
        _newStarProbability(jsonObject["spb"]),
        _starSize(jsonObject[PTY_SIZE]),
        _blendType(static_cast<TBlendType>(jsonObject[PTY_BLEND])),
        _maxSpeed(jsonObject[PTY_MAXSPEED]),
        _blurFactor(jsonObject[PTY_BLUR]),
        _musicFactor(jsonObject["msf"]),
        _skyColor(jsonObject[PTY_COLOR].as<CRGB>()),
        _useBeatColorCoding(jsonObject["fn"].as<String>() == kRgbMusicBlendStarsName)
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _palette;
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
            // MusicStar creation is driven exclusively from beat callbacks. Draw-time
            // polling and VU-based probability scaling are intentionally bypassed.
            if constexpr (std::is_same_v<StarType, MusicStar>)
            {
                while (!_pendingMusicStarColors.empty() && _allParticles.size() < cMaxStars)
                {
                    StarType newstar(_palette, _blendType, _maxSpeed * std::max(1.0f, _musicFactor), _starSize);
                    if (_pendingMusicStarColors.front() >= 0)
                        newstar.SetColorIndex(static_cast<uint8_t>(_pendingMusicStarColors.front()));
                    newstar._iPos = (int) random_range(0U, LEDStripEffect::_cLEDs - 1 - starWidth);
                    _allParticles.push_back(newstar);
                    _pendingMusicStarColors.pop_front();
                }
                return;
            }
        #endif

        for (int i = 0; i < cMaxNewStarsPerFrame; i++)
        {
            float prob = _newStarProbability / 100.0f;
            float speedMultiplier = 1.0f;

            #if ENABLE_AUDIO
                // If we have audio enabled, modulate probability and speed based on the music.
                // Only apply modulation when there is actual sound (VU > 0) — otherwise fall back to base probability.
                if (g_Analyzer.VU() > 0)
                {
                    prob += (g_Analyzer.VURatio() - 1.0f) * _musicFactor;
                    speedMultiplier = _musicFactor;
                }
            #endif

            // Clamp probability to a sane range to avoid negative or runaway values
            prob = std::clamp(prob, 0.0f, 1.0f);

            constexpr auto kProbabilitySpan = 1.0f;

            // Ensure probability is positive before rolling dice
            if (prob > 0.0f && (random_range(0.0f, kProbabilitySpan) < g_Values.AppTime.LastFrameTime() * prob))
            {
                StarType newstar(_palette, _blendType, _maxSpeed * speedMultiplier, _starSize);
                // This always starts stars on even pixel boundaries so they look like the desired width if not moving
                newstar._iPos = (int) random_range(0U, LEDStripEffect::_cLEDs - 1 - starWidth);
                _allParticles.push_back(newstar);
            }
        }
    }

    void OnBeat(const BeatInfo& beat) override
    {
        LEDStripEffect::OnBeat(beat);

        #if ENABLE_AUDIO
            if constexpr (std::is_same_v<StarType, MusicStar>)
            {
                if (beat.sequence == 0 || beat.sequence == _lastMusicBeatSequence)
                    return;

                _lastMusicBeatSequence = beat.sequence;
                QueueMusicStarsForBeat(beat, true);
            }
        #endif
    }

    void OnNearBeat(const BeatInfo& beat) override
    {
        LEDStripEffect::OnNearBeat(beat);

        #if ENABLE_AUDIO
            if constexpr (std::is_same_v<StarType, MusicStar>)
            {
                if (beat.sequence == 0 || beat.sequence == _lastMusicNearBeatSequence)
                    return;

                _lastMusicNearBeatSequence = beat.sequence;
                QueueMusicStarsForBeat(beat, false);
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
            LEDStripEffect::g().blurRows(LEDStripEffect::g().leds, MATRIX_WIDTH, MATRIX_HEIGHT, 0, _blurFactor * 255);
            #if ENABLE_AUDIO
                if constexpr (std::is_same_v<StarType, MusicStar>)
                {
                    LEDStripEffect::fadeAllChannelsToBlackBy(55);
                }
                else
            #endif
                {
                    LEDStripEffect::fadeAllChannelsToBlackBy(55 * (2.0 - g_Analyzer.VURatioFade()));
                }
        }

        for(auto i = _allParticles.begin(); i != _allParticles.end(); i++)
        {
            i->UpdatePosition();
            float fPos = i->_iPos;
            CRGB c = i->ObjectColor();
            LEDStripEffect::setPixelsFOnAllChannels(fPos - i->_objectSize / 2.0, i->_objectSize, c, true);
        }
    }
};

template <typename StarType>
class StarEffect : public StarEffectBase<StarType, StarEffect<StarType>>
{
  public:
    using StarEffectBase<StarType, StarEffect<StarType>>::StarEffectBase;
};

// NightTwinkleEffect
//
// Fills the strip with a base night color, then overlays randomly triggered
// white stars using the standard particle pre-ignition/ignition/hold/fade cycle.

class NightTwinkleEffect : public EffectWithId<NightTwinkleEffect>
{
  protected:

    class NightTwinkleStar : public ColorStar
    {
      public:
        NightTwinkleStar()
          : ColorStar(CRGB::White, 0.0f, 1.0f)
        {
        }

        float PreignitionTime() const override { return 0.0f;  }
        float IgnitionTime()    const override { return 0.05f; }
        float HoldTime()        const override { return 0.10f; }
        float FadeTime()        const override { return 0.30f; }
    };

    static constexpr const char * PTY_DENSITY = "dns";
    static constexpr const char * PTY_STARS_PER_FRAME = "spf";
    static constexpr float  kDefaultDensity           = 1.0f;
    static constexpr float  kLegacyDefaultDensity     = 0.01f;
    static constexpr size_t kDefaultStarsPerFrame     = 10;
    static constexpr float  kReferenceLEDCount        = 144.0f;

    std::deque<NightTwinkleStar> _allParticles;
    CRGB                         _baseColor;
    float                        _density;
    size_t                       _starsPerFrame;

    static float DensityFromJSON(const JsonObjectConst& jsonObject)
    {
        if (!jsonObject[PTY_DENSITY].is<float>())
            return kDefaultDensity;

        const float density = jsonObject[PTY_DENSITY].as<float>();
        if (!jsonObject[PTY_STARS_PER_FRAME].is<size_t>())
            return density / kLegacyDefaultDensity;

        return density;
    }

    void Update()
    {
        while (!_allParticles.empty() && _allParticles.front().Age() >= _allParticles.front().TotalLifetime())
            _allParticles.pop_front();

        while (_allParticles.size() > cMaxStars)
            _allParticles.pop_back();
    }

    void CreateStars()
    {
        const float stripScale = static_cast<float>(_cLEDs) / kReferenceLEDCount;
        const float requestedStars = _starsPerFrame * stripScale * std::max(0.0f, _density);
        const size_t starsToCreate = requestedStars > 0.0f
            ? std::max<size_t>(1, static_cast<size_t>(std::round(requestedStars)))
            : 0;
        if (starsToCreate == 0)
            return;

        for (size_t i = 0; i < starsToCreate && _allParticles.size() < cMaxStars; ++i)
        {
            NightTwinkleStar star;
            star._iPos = static_cast<float>(random_range(0U, _cLEDs - 1));
            _allParticles.push_back(star);
        }
    }

  public:

    NightTwinkleEffect(CRGB baseColor = CRGB(0, 0, 255), float density = kDefaultDensity, size_t starsPerFrame = kDefaultStarsPerFrame)
      : EffectWithId<NightTwinkleEffect>("NightTwinkle"),
        _baseColor(baseColor),
        _density(density),
        _starsPerFrame(starsPerFrame)
    {
    }

    NightTwinkleEffect(const JsonObjectConst& jsonObject)
      : EffectWithId<NightTwinkleEffect>(jsonObject),
        _baseColor(jsonObject[PTY_COLOR].is<CRGB>() ? jsonObject[PTY_COLOR].as<CRGB>() : CRGB(0, 0, 255)),
        _density(DensityFromJSON(jsonObject)),
        _starsPerFrame(jsonObject[PTY_STARS_PER_FRAME].is<size_t>() ? jsonObject[PTY_STARS_PER_FRAME].as<size_t>() : kDefaultStarsPerFrame)
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_COLOR] = _baseColor;
        jsonDoc[PTY_DENSITY] = _density;
        jsonDoc[PTY_STARS_PER_FRAME] = _starsPerFrame;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    void Draw() override
    {
        Update();
        CreateStars();

        fillSolidOnAllChannels(_baseColor);

        for (auto& star : _allParticles)
        {
            star.UpdatePosition();
            setPixelOnAllChannels(static_cast<int>(star._iPos), _baseColor + star.ObjectColor());
        }
    }
};

template <typename StarType> class BlurStarEffect : public StarEffectBase<StarType, BlurStarEffect<StarType>>
{
  public:

    BlurStarEffect(const CRGBPalette16 & palette, float probability = 0.2, size_t starSize = 1, TBlendType blendType = LINEARBLEND, float maxSpeed = 20.0)
        : StarEffectBase<StarType, BlurStarEffect<StarType>>(palette, probability, starSize, blendType, maxSpeed)
    {
    }

    BlurStarEffect(const JsonObjectConst& jsonObject)
        : StarEffectBase<StarType, BlurStarEffect<StarType>>(jsonObject)
    {
    }

    virtual void Clear()
    {
        StarEffect<StarType>::setAll(32,0,0);
    }
};

// TwinkleStarEffect
//
// Twinkles random colored dots on and off

class TwinkleStarEffect : public EffectWithId<TwinkleStarEffect>
{
  private:

    #define NUM_TWINKLES 100
    int buffer[NUM_TWINKLES];

  public:

    TwinkleStarEffect() : EffectWithId<TwinkleStarEffect>("Twinkle Star") {}
    TwinkleStarEffect(const JsonObjectConst& jsonObject) : EffectWithId<TwinkleStarEffect>(jsonObject) {}

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
