#pragma once
//+--------------------------------------------------------------------------
//
// File:        lanterneffect.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: lantern-style fan effects and helper particle simulation.
//---------------------------------------------------------------------------
//

#include "effects.h"
#include "effects/strip/fan_geometry.h"

/*
 * Effects intended for a train-style lantern with concentric rings of 16/12/8/1
 */

// Lantern - A candle-like effect that flickers in the center of an LED disc
//           Inspired by a candle effect I saw done by carangil

class LanternParticle
{
    const int minPeturbation = 100;
    const int maxPeterbation = 3500;
    const int perterbationIncrement = 1;
    const int maxDeviation = 100;

    int centerX = maxDeviation / 2;
    int centerY = maxDeviation / 2;

    int velocityX = 0;
    int velocityY = 0;

    int pertub = minPeturbation;
    int perturbDirection = perterbationIncrement;

    float rotation = 0.0f;

protected:

    float distance(float x1, float y1, float x2, float y2)
    {
        return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
    }

    CRGB flameColor(int val)
    {
        val = min(val, 255);
        val = max(val, 0);

        return CRGB(val, val * .30, val * .05);
    }

    // Generate a vector of how bright each of the surrounding 8 LEDs on the unit circle should be
    std::vector<float> led_brightness(float wandering_x, float wandering_y)
    {
        const float sqrt2 = std::sqrt(2);

        const std::vector<std::pair<float, float>> unit_circle_coords = {
            {1, 0},
            {1 / sqrt2, 1 / sqrt2},
            {0, 1},
            {-1 / sqrt2, 1 / sqrt2},
            {-1, 0},
            {-1 / sqrt2, -1 / sqrt2},
            {0, -1},
            {1 / sqrt2, -1 / sqrt2}
        };

        std::vector<float> brightness_values;

        for (const auto& coord : unit_circle_coords)
        {
            float d = distance(wandering_x, wandering_y, coord.first, coord.second);
            float brightness = std::max(1.0f - d, 0.0f);
            brightness_values.push_back(brightness);
        }

        return brightness_values;
    }

public:
    void Draw()
    {
        // random trigger brightness oscillation, if at least half uncalm
        int movx = 0;
        int movy = 0;

        if (pertub > (maxPeterbation / 2))
        {
            if (random(2000) < 5)
                pertub = maxPeterbation; // occasional 'bonus' wind
        }

        // random poke, intensity determined by uncalm value (0 is perfectly calm)
        movx = random(pertub >> 7) - (pertub >> 9);
        movy = random(pertub >> 7) - (pertub >> 9);

        // if reach most calm value, start moving towards uncalm
        if (pertub < minPeturbation)
            perturbDirection = perterbationIncrement;

        // if reach most uncalm value, start going towards calm
        if (pertub > maxPeterbation)
            perturbDirection = -perterbationIncrement;

        pertub += perturbDirection;

        // Move center of flame around by the current velocity
        centerX += movx + (velocityX / 7);
        centerY += movy + (velocityY / 7);

        // Enforce some range limits
        if (centerX < -maxDeviation)
        {
            centerX = -maxDeviation;
            velocityX *= -0.5;
        }

        if (centerX > maxDeviation)
        {
            centerX = maxDeviation;
            velocityX *= -0.5;
        }

        if (centerY < -maxDeviation)
        {
            centerY = -maxDeviation;
            velocityY *= -0.5;
        }

        if (centerY > maxDeviation)
        {
            centerY = maxDeviation;
            velocityY *= -0.5;
        }

        // Dampen the velocity down a fraction
        velocityX = (velocityX * 999) / 1000;
        velocityY = (velocityY * 999) / 1000;

        // Apply Hooke's law of spring motion to accelerate back towards rest/center
        velocityX -= centerX;
        velocityY -= centerY;

        rotation += 0.0;

        // Draw outer pixels in ring 2, taking advantage of low-level red response.
        float xRatio = ::map(centerX, 0.0f, maxDeviation, -1.0f, 1.0f);
        float yRatio = ::map(centerY, 0.0f, maxDeviation, -1.0f, 1.0f);

        auto brightness = led_brightness(xRatio, yRatio);
        for (int i = 0; i < 8; i++)
        {
            CRGB pixelColor = flameColor(255 * brightness[i]);
            pixelColor.fadeToBlackBy(255 * (3.0 - brightness[i]));
            DrawRingPixels(i, 1, pixelColor, 0, 2, true);
        }

        // Draw center pixel dimmed by distance from center.
        CRGB centerColor = CRGB(255, 12, 0);
        centerColor.fadeToBlackBy(distance(xRatio, yRatio, 0, 0) * 128);
        DrawRingPixels(0, 1.0, centerColor, 0, 3);

        debugV("X,Y = %f, %f\n", xRatio, yRatio);
    }
};

class LanternEffect : public EffectWithId<LanternEffect>
{
private:
    static const int _maxParticles = 1;
    LanternParticle _particles[_maxParticles];

public:
    LanternEffect() : EffectWithId<LanternEffect>("LanternEffect") {}

    LanternEffect(const JsonObjectConst& jsonObject) : EffectWithId<LanternEffect>(jsonObject) {}

    size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }

    void Draw() override
    {
        fadeAllChannelsToBlackBy(20);
        for (int i = 0; i < _maxParticles; i++)
            _particles[i].Draw();
    }
};
