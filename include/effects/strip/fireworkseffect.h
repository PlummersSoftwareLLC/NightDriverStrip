//+--------------------------------------------------------------------------
//
// File:        fireworkseffect.h
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
//    A particle-based fireworks effect for strip-style outputs.
//
//---------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <array>

#include "ledstripeffect.h"
#include "musiceffect.h"

#if ENABLE_AUDIO

class FireworksEffect : public BeatEffectBase, public EffectWithId<FireworksEffect>
{
  private:

    static constexpr const char * JSON_MAX_SPEED = "fms";
    static constexpr const char * JSON_NEW_PARTICLE_PROBABILITY = "fnp";
    static constexpr const char * JSON_PREIGNITION_TIME = "fpt";
    static constexpr const char * JSON_IGNITION_TIME = "fig";
    static constexpr const char * JSON_HOLD_TIME = "fht";
    static constexpr const char * JSON_FADE_TIME = "fft";
    static constexpr const char * JSON_PARTICLE_SIZE = "fsz";
    static constexpr size_t MAX_PARTICLE_CAPACITY = 192;

    struct Particle
    {
        CRGB _baseColor = CRGB::Black;
        double _birthTime = 0.0;
        float _velocity = 0.0f;
        float _position = 0.0f;

        Particle() = default;

        Particle(const CRGB& color, float position, float velocity)
            : _baseColor(color),
              _birthTime(g_Values.AppTime.FrameStartTime()),
              _velocity(velocity),
              _position(position)
        {
        }

        float Age() const
        {
            return static_cast<float>(g_Values.AppTime.FrameStartTime() - _birthTime);
        }

        void Update(float deltaTime)
        {
            _position += _velocity * deltaTime;
            _velocity -= 2.0f * _velocity * deltaTime;
        }
    };

    std::array<Particle, MAX_PARTICLE_CAPACITY> _particles{};
    size_t _particleCapacity = 0;
    size_t _particleHead = 0;
    size_t _particleCount = 0;

    float _maxSpeed;
    float _newParticleProbability;
    float _particlePreignitionTime;
    float _particleIgnition;
    float _particleHoldTime;
    float _particleFadeTime;
    float _particleSize;

    size_t ParticleIndex(size_t logicalIndex) const
    {
        return (_particleHead + logicalIndex) % _particleCapacity;
    }

    Particle& ParticleAt(size_t logicalIndex)
    {
        return _particles[ParticleIndex(logicalIndex)];
    }

    const Particle& ParticleAt(size_t logicalIndex) const
    {
        return _particles[ParticleIndex(logicalIndex)];
    }

    bool PushParticle(const Particle& particle)
    {
        if (_particleCapacity == 0)
            return false;

        if (_particleCount >= _particleCapacity)
            return false;

        const size_t tail = (_particleHead + _particleCount) % _particleCapacity;
        _particles[tail] = particle;
        ++_particleCount;
        return true;
    }

    void PopFrontParticle()
    {
        if (_particleCount == 0 || _particleCapacity == 0)
            return;

        _particleHead = (_particleHead + 1) % _particleCapacity;
        --_particleCount;
    }

  public:

    void SpawnFirework(int count = 1)
    {
        if (_particleCapacity == 0 || _cLEDs == 0)
            return;

        const size_t ledCount = _cLEDs;
        const float maxSpeed = std::max(0.0f, _maxSpeed);

        for (int i = 0; i < count; ++i)
        {
            const float startPos = ledCount > 1
                ? random_range(0.0f, static_cast<float>(ledCount - 1))
                : 0.0f;
            const CRGB color = CHSV(random8(), 255, 255);
            const int particleCount = random_range(10, 50);
            const float velocityMultiplier = random_range(1.0f, 3.0f);
            const float maxVelocity = maxSpeed * velocityMultiplier;
            const size_t availableSlots = _particleCapacity - _particleCount;
            const size_t burstCount = std::min<size_t>(static_cast<size_t>(particleCount), availableSlots);

            for (size_t j = 0; j < burstCount; ++j)
            {
                if (!PushParticle(Particle(color, startPos, random_range(-maxVelocity, maxVelocity))))
                    break;
            }
        }
    }

    void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        SpawnFirework(8);
        Serial.printf("Beat detected! Elapsed: %0.2f, Span: %0.2f\n", elapsed, span);
    }

  public:

    FireworksEffect(const String &name = "Fireworks",
                    float maxSpeed = 175.0f,
                    float newParticleProbability = 30.0f,
                    float particlePreignitionTime = 0.0f,
                    float particleIgnition = 0.2f,
                    float particleHoldTime = 0.0f,
                    float particleFadeTime = 2.0f,
                    float particleSize = 1.0f)
        : BeatEffectBase(1.50, 0.10),
          EffectWithId<FireworksEffect>(name),
          _maxSpeed(maxSpeed),
          _newParticleProbability(newParticleProbability),
          _particlePreignitionTime(particlePreignitionTime),
          _particleIgnition(particleIgnition),
          _particleHoldTime(particleHoldTime),
          _particleFadeTime(particleFadeTime),
          _particleSize(particleSize)
    {
    }

    FireworksEffect(const JsonObjectConst& jsonObject)
        : BeatEffectBase(1.50, 0.10),
          EffectWithId<FireworksEffect>(jsonObject),
          _maxSpeed(jsonObject[JSON_MAX_SPEED] | 175.0f),
          _newParticleProbability(jsonObject[JSON_NEW_PARTICLE_PROBABILITY] | 1.0f),
          _particlePreignitionTime(jsonObject[JSON_PREIGNITION_TIME] | 0.0f),
          _particleIgnition(jsonObject[JSON_IGNITION_TIME] | 0.2f),
          _particleHoldTime(jsonObject[JSON_HOLD_TIME] | 0.0f),
          _particleFadeTime(jsonObject[JSON_FADE_TIME] | 2.0f),
          _particleSize(jsonObject[JSON_PARTICLE_SIZE] | 1.0f)
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[JSON_MAX_SPEED] = _maxSpeed;
        jsonDoc[JSON_NEW_PARTICLE_PROBABILITY] = _newParticleProbability;
        jsonDoc[JSON_PREIGNITION_TIME] = _particlePreignitionTime;
        jsonDoc[JSON_IGNITION_TIME] = _particleIgnition;
        jsonDoc[JSON_HOLD_TIME] = _particleHoldTime;
        jsonDoc[JSON_FADE_TIME] = _particleFadeTime;
        jsonDoc[JSON_PARTICLE_SIZE] = _particleSize;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        _particleCapacity = std::min<size_t>(_cLEDs, MAX_PARTICLE_CAPACITY);
        _particleHead = 0;
        _particleCount = 0;

        return true;
    }

    size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    void Draw() override
    {
        if (_particleCapacity == 0 || _cLEDs == 0)
            return;

        ProcessAudio();

        const float deltaTime = std::clamp(static_cast<float>(g_Values.AppTime.LastFrameTime()), 0.0f, 0.25f);
        const size_t ledCount = _cLEDs;
        const float spawnProbability = std::clamp(_newParticleProbability, 0.0f, 10.0f) * g_Analyzer.VURatio();
        const float preignitionTime = std::max(0.0f, _particlePreignitionTime);
        const float ignitionTime = std::max(0.0f, _particleIgnition);
        const float holdTime = std::max(0.0f, _particleHoldTime);
        const float fadeTime = std::max(0.0f, _particleFadeTime);
        const float particleSizeScale = std::clamp(_particleSize, 0.05f, 20.0f);
        const int spawnAttempts = std::max(10, static_cast<int>(ledCount / 50));

        for (int i = 0; i < spawnAttempts; ++i)
        {
            if (random_range(0.0f, 1.0f) >= spawnProbability * 0.005f)
                continue;

            SpawnFirework(5);
        }

        fadeAllChannelsToBlackBy(64);

        const float maxAge = preignitionTime + ignitionTime + holdTime + fadeTime;
        while (_particleCount > 0 && ParticleAt(0).Age() > maxAge)
            PopFrontParticle();

        const float preigniteEnd = preignitionTime;
        const float ignitionEnd = preigniteEnd + ignitionTime;
        const float holdEnd = ignitionEnd + holdTime;
        const float fadeEnd = holdEnd + fadeTime;
        const float baseParticleSize = std::max(1.0f, static_cast<float>(ledCount) / 500.0f) * particleSizeScale;

        for (size_t i = 0; i < _particleCount; ++i)
        {
            auto& particle = ParticleAt(i);
            particle.Update(deltaTime);

            const float age = particle.Age();
            float fade = 0.0f;
            CRGB color = particle._baseColor;

            if (age < preigniteEnd)
            {
                if (preigniteEnd > 0.0f)
                {
                    float t = std::clamp(age / preigniteEnd, 0.0f, 1.0f);
                    color = CRGB::White;
                    color.fadeToBlackBy(static_cast<uint8_t>((1.0f - t) * 255.0f));
                }
                else
                {
                    color = CRGB::White;
                }
            }
            else if (age < ignitionEnd)
            {
                color = CRGB::White;
            }
            else if (age < holdEnd)
            {
                color = particle._baseColor;
            }
            else if (age < fadeEnd)
            {
                if (fadeTime > 0.0f)
                    fade = std::clamp((age - holdEnd) / fadeTime, 0.0f, 1.0f);
                else
                    fade = 1.0f;

                color.fadeToBlackBy(static_cast<uint8_t>(fade * 255.0f));
            }
            else
            {
                continue;
            }

            const float currentSize = std::clamp(std::max(1.0f, baseParticleSize * (1.0f - fade)), 1.0f, static_cast<float>(ledCount));
            if (particle._position + currentSize < 0.0f || particle._position >= static_cast<float>(ledCount))
                continue;

            setPixelsOnAllChannels(particle._position, currentSize, color, false);
        }
    }
};

#endif // ENABLE_AUDIO
