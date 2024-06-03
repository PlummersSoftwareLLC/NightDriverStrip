//+--------------------------------------------------------------------------
//
// File:        Particles.h
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
//   Classes for moving and fading little render objects over time,
//   used as a base for the star and insulator effects
//
// History:     Jul-7-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#pragma once

#include "effects.h"

// Lifespan
//
// Base class that knows only when it was created, so it can later tell you old it is, which is
// used by the fading classes

class Lifespan
{
  protected:

    const double _birthTime;

  public:

    Lifespan() :_birthTime(g_Values.AppTime.FrameStartTime())
    {
    }

    virtual ~Lifespan()
    {}

    double Age() const
    {
        return g_Values.AppTime.FrameStartTime() - _birthTime;
    }

    virtual double TotalLifetime() const = 0;
};

// MovingObject
//
// Keeps track of a position, velocity, and maximum speed, mo
// and manage it's velocity (postion drift).  Updates its position
// automatically based on speed if called every frame.

class MovingObject
{
protected:

    float                       _velocity;
    float                       _maxSpeed;

public:

    float                       _iPos;

    MovingObject(float maxSpeed = 0.25) : _maxSpeed(maxSpeed)
    {
        // Return a random value between -maxSpeed and +maxSpeed

        _velocity = random_range(0.0f, _maxSpeed * 2) - _maxSpeed;
    }

    virtual ~MovingObject()
    {}

    virtual void UpdatePosition()
    {
        _iPos += _velocity * g_Values.AppTime.LastFrameTime();
    }
};

// FadingObject
//
// Based on its age and provided information about how long each life stage lasts, provides a
// 'FadeoutAmount' function that indicates how much the particle should be dimmed from aging

class FadingObject : public Lifespan
{
  protected:

    virtual float PreignitionTime() const         { return 0.0f;  }
    virtual float IgnitionTime()    const         { return 0.5f;  }
    virtual float HoldTime()        const         { return 1.00f; }
    virtual float FadeTime()        const         { return 1.5f;  }

  public:

    double TotalLifetime() const override
    {
        return PreignitionTime() + IgnitionTime() + HoldTime() + FadeTime();
    }

    virtual float FadeoutAmount() const
    {
        float age = Age();
        if (age < 0)
            age = 0;

        if (age < PreignitionTime() && PreignitionTime() != 0.0f)
            return 1.0 - (age / PreignitionTime());
        age -= PreignitionTime();
        if (age < IgnitionTime() && IgnitionTime() != 0.0f)
            return (age / IgnitionTime());
        age -= IgnitionTime();
        if (age < HoldTime())
            return 0.0f;                                                // Just born
        if (age > HoldTime() + FadeTime())
            return 1.0f;                                                // Black hole, all faded out
        age -= (HoldTime());
            return (age / FadeTime());                                  // Fading star
    }
};

class FadingCountDownObject : public FadingObject
{
  protected:

    unsigned long               _maxValue;

  public:

    FadingCountDownObject(unsigned long maxvalue)
      : _maxValue(maxvalue)
    {
    }

    virtual unsigned long CurrentCountdown()
    {
        return map(Age(), 0, TotalLifetime(), _maxValue, 0);
    }
};

// FadingColoredObject
//
// Object that flashes white during ignition and fades color throughout the the rest of its lifetime

class FadingColoredObject : public FadingObject
{
  protected:

    CRGB                         _baseColor;

  public:

    FadingColoredObject(CRGB baseColor)
      : _baseColor(baseColor)
    {
    }

    virtual CRGB ObjectColor() const
    {
        if (Age() >= PreignitionTime() && Age() < IgnitionTime() + PreignitionTime())
        {
            CRGB c = CRGB::White;
            c.fadeToBlackBy(255 - ((Age() - PreignitionTime()) / IgnitionTime() * 255));
            return c + _baseColor;
        }

        CRGB c = _baseColor;
        fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
        return c;
    }
};

// FadingPaletteObject
//
// Uses a palette plus an index to generate its ObjectColor, fades over its lifetime, flashes white at ignition

class FadingPaletteObject : public FadingObject
{
  protected:

    const CRGBPalette16 & _palette;
    const TBlendType       _blendType = NOBLEND;
    uint8_t                _colorIndex;

  public:

    FadingPaletteObject(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, uint8_t colorIndex =  0)
      : _palette(palette),
        _blendType(blendType),
        _colorIndex(colorIndex)
    {
    }

    virtual CRGB ObjectColor() const
    {
        CRGB c = ColorFromPalette(_palette, _colorIndex, 255, _blendType);
        if (Age() < PreignitionTime())
        {
            fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
            return c;
        }
        else if (Age() >= PreignitionTime() && (Age() < (IgnitionTime() + PreignitionTime())))
        {
            CRGB c = CRGB::White;
            fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
            return c;
        }

        fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
        return c;
    }

    virtual void SetColorIndex(uint8_t index)
    {
        _colorIndex = index;
    }

    virtual uint8_t GetColorIndex() const
    {
        return _colorIndex;
    }
};


// MovingFadingColoredObject
//
// Moves, fades over it's lifetime, based on palette+index color

class MovingFadingPaletteObject: public FadingPaletteObject, public MovingObject
{
  public:

    MovingFadingPaletteObject(const CRGBPalette16 & palette, TBlendType blendType = NOBLEND, float maxSpeed = 1.0, uint8_t colorIndex = random8())
      : FadingPaletteObject(palette, blendType, colorIndex),
        MovingObject(maxSpeed)
    {
    }
};

// MovingFadingColoredObject
//
// Moves, fades over it's lifetime, based on CRGB color

class MovingFadingColoredObject: public FadingColoredObject, public MovingObject
{
  public:

    MovingFadingColoredObject(CRGB baseColor, float maxSpeed = 1.0)
      : FadingColoredObject(baseColor),
        MovingObject(maxSpeed)
    {
    }
};

// ObjectSize
//
// Super-simple class which holds a size

class ObjectSize
{
    public:
      float _objectSize;

      ObjectSize(float size = 1.0)
        :_objectSize(size)
      {
      }
};

class DrawableParticle : public Lifespan
{
  protected:

  public:
     virtual void Render(const std::vector<std::shared_ptr<GFXBase>>& _GFX) = 0;
};

template <typename Type = DrawableParticle> class ParticleSystem
{
  protected:

    std::deque<Type> _allParticles;

    // Once per frame we are called to update all particles, which includes aging out old ones

  public:

    ParticleSystem<Type>()
    {
    }

    virtual void Render(const std::vector<std::shared_ptr<GFXBase>>& _gfx)
    {
        debugV("ParticleSystemEffect::Draw for %d particles", _allParticles.size());

        while (_allParticles.size() > 0 && _allParticles.front().Age() >= _allParticles.front().TotalLifetime())
            _allParticles.pop_front();

        while (_allParticles.size() > _gfx[0]->GetLEDCount())
            _allParticles.pop_front();

        for(auto i = _allParticles.begin(); i != _allParticles.end(); i++)
            i->Render(_gfx);
    }
};

class RingParticle : public FadingColoredObject
{
  protected:

    int             _iInsulator;
    int             _iRing;
    float           _ignitionTime;
    float           _fadeTime;

  public:

    RingParticle(int iInsulator, int iRing, CRGB color, float ignitionTime = 0.0f, float fadeTime = 1.0f)
      :  FadingColoredObject(color),
         _iInsulator(iInsulator),
         _iRing(iRing),
         _ignitionTime(ignitionTime),
         _fadeTime(fadeTime)
    {
        assert(iRing <= NUM_RINGS);
        assert(iInsulator < NUM_FANS);
        debugV("Creating particle at insulator %d", iInsulator);
    }

    virtual void Render(const std::vector<std::shared_ptr<GFXBase>>& _GFX)
    {
        debugV("Particle Render at insulator %d", _iInsulator);

        if (_iInsulator < 0)    // -1 is a major beat, all insulators
        {
          CRGB c = ObjectColor();

          //c.fadeToBlackBy(192);

          for (int i = 0; i < NUM_FANS; i++)
          {

            FillRingPixels(c, i, _iRing);
          }
        }
        else    // Individual ring for a minor beat
        {
          FillRingPixels(ObjectColor(), _iInsulator, _iRing);
        }
    }

    float PreignitionTime() const override         { return 0.0f;          }
    float IgnitionTime()    const override         { return _ignitionTime; }
    float HoldTime()        const override         { return 0.0f;          }
    float FadeTime()        const override         { return _fadeTime;     }
};


#if ENABLE_AUDIO
class ColorBeatWithFlash : public BeatEffectBase, public ParticleSystem<RingParticle>, LEDStripEffect
{
    int _iLastInsulator = 0;
    CRGB _baseColor = CRGB::Black;

  public:

    ColorBeatWithFlash(const String & strName) : BeatEffectBase(), ParticleSystem<RingParticle>(), LEDStripEffect(EFFECT_STRIP_COLOR_BEAT_WITH_FLASH, strName)
    {
    }

    ColorBeatWithFlash(const JsonObjectConst& jsonObject) : BeatEffectBase(), ParticleSystem<RingParticle>(), LEDStripEffect(jsonObject)
    {
    }

    virtual void LightInsulator(int iInsulator, int iRing, CRGB color, bool bMajor)
    {
      debugV("MusicalInsulatorEffect2 LightInsulator for Insulator %d", iInsulator);

      RingParticle newparticle(iInsulator, iRing, color, !bMajor ? 0.05 : 0.0, 0.75);
      _allParticles.push_back(newparticle);
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
      for (int pass = 0; pass < 1; pass++)
      {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;

        CRGB c = CHSV(beatsin8(4), 255, 127.5*g_Analyzer._VURatio);
        CRGB r = RandomSaturatedColor();
        LightInsulator(bMajor ? - 1: iInsulator, 0, bMajor ? r : c, bMajor);
      }
    }

    virtual void Draw() override
    {
      // We are inheriting from both the insulator music beat effect and a particle system effect, and both need
      //
      setAllOnAllChannels(0,0,0);

      uint8_t v = 16  * g_Analyzer._VURatio;
      _baseColor += CRGB(CHSV(beatsin8(24), 255, v));
      _baseColor.fadeToBlackBy(8 * g_Analyzer._VURatio);
      setAllOnAllChannels(_baseColor.r, _baseColor.g, _baseColor.b);
      BeatEffectBase::ProcessAudio();
      ParticleSystem<RingParticle>::Render(_GFX);
    }
};

class ColorBeatOverRed : public LEDStripEffect, public BeatEffectBase, public ParticleSystem<RingParticle>
{
    int  _iLastInsulator = 0;
    CRGB _baseColor = CRGB::Black;

  public:

    ColorBeatOverRed(const String & strName)
      : LEDStripEffect(EFFECT_STRIP_COLOR_BEAT_OVER_RED, strName),
        BeatEffectBase(1.75, 0.2),
        ParticleSystem<RingParticle>()
    {
    }

    ColorBeatOverRed(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        BeatEffectBase(1.75, 0.2),
        ParticleSystem<RingParticle>()
    {
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;

        if (bMajor && span >= 1.999)
          iInsulator = -1;

        float fadetime = min(5.0, elapsed * 1.5);   // Cap it at 5 seconds so we don't get ultra-long beats resulting from delays
        float flashtime = 0;

        _allParticles.push_back(RingParticle(iInsulator, 0, RandomSaturatedColor(), flashtime, fadetime));
    }

    virtual void Draw() override
    {
      ProcessAudio();

      // We are inheriting from both the insulator music beat effect and a particle system effect, and both need a chance
      // to draw.  BeatEffectBase doesn't draw anything directly, but it does call us back at HandleBeat when needed.  We
      // also have to update and render the particle system, which does the actual pixel drawing.  We clear the scene ever
      // pass and rely on the fade effects of the particles to blend the

      float amount = g_Analyzer._VU / MAX_VU;

      _baseColor = CRGB(500 * amount, 0, 0);
      setAllOnAllChannels(_baseColor.r, _baseColor.g, _baseColor.b);
      ParticleSystem<RingParticle>::Render(_GFX);

    }
};
#endif

class SpinningPaletteRingParticle : public FadingObject
{
  protected:

          int             _iInsulator;
          int             _iRing;
    const CRGBPalette16  _palette;
          int             _length;
          int             _start;
    const float           _density;
    const float           _paletteSpeed;
    const float           _LEDSPerSecond;
    const float           _lightSize;
    const float           _gapSize;
    const TBlendType      _blend;
    const bool            _bErase;
          float           _startIndex;
          float           _paletteIndex;
    const float           _brightness;
    const float           _ignitionTime;


  public:

    SpinningPaletteRingParticle(
                  int                    iInsulator,
                  int                    iRing,
                  const CRGBPalette16 & palette,
                  float                  density = 4.0,
                  float                  paletteSpeed = 0.25,
                  float                  ledsPerSecond = 0.1,
                  float                  lightSize = 1,
                  float                  gapSize = 0,
                  TBlendType             blend = LINEARBLEND,
                  bool                   bErase = true,
                  float                  brightness = 1.0f,
                  float                  ignitionTime = 0.0)
      :  _iInsulator(iInsulator),
         _iRing(iRing),
         _palette(palette),
         _density(density),
         _paletteSpeed(paletteSpeed),
         _LEDSPerSecond(ledsPerSecond),
         _lightSize(lightSize),
         _gapSize(gapSize),
         _blend(blend),
         _bErase(bErase),
         _startIndex(0.0f),
         _paletteIndex(0.0f),
         _brightness(brightness),
         _ignitionTime(ignitionTime)
    {
        assert(iRing <= NUM_RINGS);
        assert(iInsulator < NUM_FANS);
        debugV("Creating particle at insulator %d", iInsulator);

        // REVIEW(davepl): I'm not sure what I was doing here.  I think it allows you to pass 0 for speed and always start at 0 index?

        _paletteIndex += beatsin16(4, 0, 255) * paletteSpeed;
        _startIndex += beatsin16(4, 0, 255) * ledsPerSecond;

        _start = iInsulator * FAN_SIZE;     // Move to correct insulator
        for (int i = 0; i < iRing; i++)     // Move to ring within the insulator
          _start += g_aRingSizeTable[i];

        _length = g_aRingSizeTable[iRing];    // Length is size of this particular ring
    }

    virtual void Render(const std::vector<std::shared_ptr<GFXBase>>& _GFX)
    {
        debugV("Particle Render at insulator %d", _iInsulator);

        if (_bErase)
          _GFX[0]->setPixelsF(_start, _length, CRGB::Black, false);

        float deltaTime = g_Values.AppTime.LastFrameTime();
        float increment = (deltaTime * _LEDSPerSecond);
        const int totalSize = _gapSize + _lightSize + 1;
        _startIndex   = totalSize > 1 ? fmodf(_startIndex + increment, totalSize) : 0;

        // A single color step in a palette is 32 increments.  There are 256 total in a palette, and 144 pixels per meter typical, so this
        // scaling yields a color rotation of "one full palette per meter" by default.  We go backwards (-1) to match pixel scrolling direction.

        _paletteIndex = _paletteIndex - (deltaTime * _paletteSpeed * _density);

        float iColor = 0; // fmodf(_paletteIndex + _startIndex * _density, 256);

        if (_gapSize == 0)
        {
          for (int i = _start; i < _start+_length; i+=_lightSize)
          {
            iColor = fmodf(iColor + _density, 256);
            _GFX[0]->setPixelsF(i, _lightSize, ColorFromPalette(_palette, iColor, 255 - 255 * FadeoutAmount(), _blend), true);
          }
        }
        else
        {
          // Start far enough "back" to have one off-strip light and gap, and then we need to draw at least as far as the last light.
          // This prevents sticks of light from "appearing" or "disappearing" at the ends

          for (float i = _start-totalSize; i < _start+_length+_lightSize; i++)
          {
              // We look for each pixel where we cross an even multiple of the light+gap size, which means it's time to start the drawing
              // of the light here

              iColor = fmodf(iColor + _density, 256);
              int index = fmodf(i, totalSize);
              if (index == 0)
              {
                  CRGB c = ColorFromPalette(_palette, iColor, 255 * _brightness * FadeoutAmount(), _blend);
                  if (i + _startIndex > _start)
                    _GFX[0]->setPixelsF(i+_startIndex, _lightSize, c, true);
              }
          }
        }

        if (Age() < IgnitionTime() + PreignitionTime() && Age() >= PreignitionTime())
          _GFX[0]->setPixelsF(_start + random(0, _length), 1, CRGB::White, true);
    }

    float PreignitionTime() const override         { return 0.0f;          }
    float IgnitionTime() const override            { return _ignitionTime; }
    float HoldTime() const override                { return 0.0f;          }
    float FadeTime() const override                { return 1.00;          }
};


class HotWhiteRingParticle : public FadingObject
{
  protected:

    int             _iInsulator;
    int             _iRing;
    float           _ignitionTime;
    float           _fadeTime;

  public:

    HotWhiteRingParticle(int iInsulator, int iRing, float ignitionTime = 0.25f, float fadeTime = 1.0f)
      :  _iInsulator(iInsulator),
         _iRing(iRing),
         _ignitionTime(ignitionTime),
         _fadeTime(fadeTime)
    {
        assert(iRing <= NUM_RINGS);
        assert(iInsulator < NUM_FANS);
        debugV("Creating particle at insulator %d", iInsulator);
    }

    virtual void Render(const std::vector<std::shared_ptr<GFXBase>>& _GFX)
    {
        debugV("Particle Render at insulator %d", _iInsulator);

        CRGB c = CRGB::White;

        if (Age() >= PreignitionTime() && Age() < IgnitionTime() + PreignitionTime())
        {
            c = CRGB::White;
            //c.fadeToBlackBy(255 - ((Age() - PreignitionTime()) / IgnitionTime() * 255));
        }
        else
        {
          float age = Age() - PreignitionTime() - IgnitionTime();

          uint8_t temperature = 255 * (1.0 - (age/FadeTime()));
          uint8_t t192 = round((temperature/255.0)*191);

          // calculate ramp up from
          uint8_t heatramp = t192 & 0x3F; // 0..63
          heatramp <<= 2; // scale up to 0..252

          if( t192 > 0x80)                      // hottest
              c = CRGB(255, 255, heatramp);
          else if( t192 > 0x40 )                // middle
              c = CRGB( 255, heatramp, 0);
          else                                  // coolest
              c = CRGB( heatramp, 0, 0);
          fadeToBlackBy(&c, 1, 255 * FadeoutAmount());
        }

        if (_iInsulator < 0)    // -1 is a major beat, all insulators
        {
            for (int i = 0; i < NUM_FANS; i++)
            {
              FillRingPixels(c, i, _iRing);
            }
        }
        else    // Individual ring for a minor beat
        {
          FillRingPixels(c, _iInsulator, _iRing);
        }
    }

    float PreignitionTime() const override         { return 0.0f;          }
    float IgnitionTime()    const override         { return _ignitionTime; }
    float HoldTime()        const override         { return 0.0f;          }
    float FadeTime()        const override         { return _fadeTime;     }
};

#if ENABLE_AUDIO


class MoltenGlassOnVioletBkgnd : public LEDStripEffect, public BeatEffectBase, public ParticleSystem<SpinningPaletteRingParticle>
{
    int                    _iLastInsulator = 0;
    const CRGBPalette16 & _Palette;
    CRGB _baseColor = CRGB::Black;

  public:

    MoltenGlassOnVioletBkgnd(const String & strName, const CRGBPalette16 & Palette)
      : LEDStripEffect(EFFECT_STRIP_MOLTEN_GLASS_ON_VIOLET_BKGND, strName),
        BeatEffectBase(1.50, 0.05),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(Palette)
    {
    }

    MoltenGlassOnVioletBkgnd(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        BeatEffectBase(1.50, 0.05),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>())
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(LEDStripEffect::_jsonSize + 512);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _Palette;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }


    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;

        switch (random(10))
        {
          case 0:
            _allParticles.push_back(SpinningPaletteRingParticle(0, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(2, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(4, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            break;

          case 1:
            _allParticles.push_back(SpinningPaletteRingParticle(1, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(3, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            break;

          case 2:
            _allParticles.push_back(SpinningPaletteRingParticle(0, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(1, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(2, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(3, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(4, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            break;

          default:
            _allParticles.push_back(SpinningPaletteRingParticle(iInsulator, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            break;
        }
    }

    virtual void Draw() override
    {
      // We are inheriting from both the insulator music beat effect and a particle system effect, and both need a chance
      // to draw.  BeatEffectBase doesn't draw anything directly, but it does call us back at HandleBeat when needed.  We
      // also have to update and render the particle system, which does the actual pixel drawing.  We clear the scene ever
      // pass and rely on the fade effects of the particles to blend the

      uint8_t v = 16  * g_Analyzer._VURatio;
      _baseColor += CRGB(CHSV(200, 255, v));
      _baseColor.fadeToBlackBy((min(255.0, 1000.0 * g_Values.AppTime.LastFrameTime())));
      setAllOnAllChannels(_baseColor.r, _baseColor.g, _baseColor.b);

      BeatEffectBase::ProcessAudio();
      ParticleSystem<SpinningPaletteRingParticle>::Render(_GFX);
    }
};

class NewMoltenGlassOnVioletBkgnd : public LEDStripEffect, public BeatEffectBase, public ParticleSystem<SpinningPaletteRingParticle>
{
    int  _iLastInsulator = 0;
    const CRGBPalette16 & _Palette;
    CRGB _baseColor = CRGB::Black;

  public:

    NewMoltenGlassOnVioletBkgnd(const String & strName, const CRGBPalette16 & Palette)
      : LEDStripEffect(EFFECT_STRIP_NEW_MOLTEN_GLASS_ON_VIOLET_BKGND, strName),
        BeatEffectBase(1.0, 0.25 ),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(Palette)
    {
    }

    NewMoltenGlassOnVioletBkgnd(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        BeatEffectBase(1.0, 0.25 ),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>())
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(LEDStripEffect::_jsonSize + 512);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _Palette;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;

        switch (random(10))
        {
          case 0:
            _allParticles.push_back(SpinningPaletteRingParticle(0, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(2, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(4, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            break;

          case 1:
            _allParticles.push_back(SpinningPaletteRingParticle(1, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(3, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            break;

          case 2:
            _allParticles.push_back(SpinningPaletteRingParticle(0, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(1, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(2, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(3, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            _allParticles.push_back(SpinningPaletteRingParticle(4, 0, _Palette, 2, 50, -0.5, 1, 0, LINEARBLEND, true, 1.0, 0));
            break;

          default:
            _allParticles.push_back(SpinningPaletteRingParticle(iInsulator, 0, _Palette, 256.0/FAN_SIZE, 0, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, 0));
            break;
        }
    }

    virtual void Draw() override
    {
      ProcessAudio();
      // We are inheriting from both the insulator music beat effect and a particle system effect, and both need a chance
      // to draw.  BeatEffectBase doesn't draw anything directly, but it does call us back at HandleBeat when needed.  We
      // also have to update and render the particle system, which does the actual pixel drawing.  We clear the scene ever
      // pass and rely on the fade effects of the particles to blend the

       uint8_t v = 16  * g_Analyzer._VURatio;
      _baseColor += CRGB(CHSV(200, 255, v));
      _baseColor.fadeToBlackBy((min(255.0, 1000.0 * g_Values.AppTime.LastFrameTime())));
      setAllOnAllChannels(_baseColor.r, _baseColor.g, _baseColor.b);

      ParticleSystem<SpinningPaletteRingParticle>::Render(_GFX);
    }
};

class SparklySpinningMusicEffect : public LEDStripEffect, public BeatEffectBase, public ParticleSystem<SpinningPaletteRingParticle>
{
    int  _iLastInsulator = 0;
    const CRGBPalette16 & _Palette;
    CRGB _baseColor = CRGB::Black;

  public:

    SparklySpinningMusicEffect(const String & strName, const CRGBPalette16 & Palette)
      : LEDStripEffect(EFFECT_STRIP_SPARKLY_SPINNING_MUSIC, strName), BeatEffectBase(), ParticleSystem<SpinningPaletteRingParticle>(), _Palette(Palette)
    {
    }

    SparklySpinningMusicEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        BeatEffectBase(),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>())
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(LEDStripEffect::_jsonSize + 512);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _Palette;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;

        _allParticles.push_back(SpinningPaletteRingParticle(iInsulator, 0, _Palette, 1, 1.0, 1.0, 1, 0, NOBLEND, true, 1.0, min(0.15f, elapsed/2)));
    }

    virtual void Draw() override
    {
      // We are inheriting from both the insulator music beat effect and a particle system effect, and both need a chance
      // to draw.  BeatEffectBase doesn't draw anything directly, but it does call us back at HandleBeat when needed.  We
      // also have to update and render the particle system, which does the actual pixel drawing.  We clear the scene ever
      // pass and rely on the fade effects of the particles to blend the

      uint8_t v = 32  * g_Analyzer._VURatio;
      _baseColor += CRGB(CHSV(beatsin8(1), 255, v));
      _baseColor.fadeToBlackBy((min(255.0, 2500.0 * g_Values.AppTime.LastFrameTime())));
      setAllOnAllChannels(_baseColor.r, _baseColor.g, _baseColor.b);

      BeatEffectBase::ProcessAudio();
      ParticleSystem<SpinningPaletteRingParticle>::Render(_GFX);
      delay(20);
    }
};

class MusicalHotWhiteInsulatorEffect : public LEDStripEffect, public BeatEffectBase, public ParticleSystem<HotWhiteRingParticle>
{
    int  _iLastInsulator = 0;
    CRGB _baseColor      = CRGB::Black;

  public:

    MusicalHotWhiteInsulatorEffect(const String & strName) : LEDStripEffect(EFFECT_STRIP_MUSICAL_HOT_WHITE_INSULATOR, strName), BeatEffectBase(), ParticleSystem<HotWhiteRingParticle>()
    {
    }

    MusicalHotWhiteInsulatorEffect(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject), BeatEffectBase(), ParticleSystem<HotWhiteRingParticle>()
    {
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;

        _allParticles.push_back(HotWhiteRingParticle(iInsulator, 0, 0.25, 0.75));
    }

    virtual void Draw() override
    {
      // We are inheriting from both the insulator music beat effect and a particle system effect, and both need a chance
      // to draw.  BeatEffectBase doesn't draw anything directly, but it does call us back at HandleBeat when needed.  We
      // also have to update and render the particle system, which does the actual pixel drawing.  We clear the scene ever
      // pass and rely on the fade effects of the particles to blend the

      uint8_t v = 32  * g_Analyzer._VURatio;
      _baseColor += CRGB(CHSV(beatsin8(1), 255, v));
      _baseColor.fadeToBlackBy((min(255.0,1000.0 * g_Values.AppTime.LastFrameTime())));
      setAllOnAllChannels(_baseColor.r, _baseColor.g, _baseColor.b);
      setAllOnAllChannels(0,0,0);

      BeatEffectBase::ProcessAudio();
      ParticleSystem<HotWhiteRingParticle>::Render(_GFX);
      delay(20);
    }
};
#endif

