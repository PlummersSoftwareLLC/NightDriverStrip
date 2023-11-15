#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/2007-amber-rain

class Circle
{
  public:
    float thickness = 3.0;
    long startTime;
    uint16_t offset;
    int16_t centerX;
    int16_t centerY;
    int hue;
    int bpm = 10;

    void move()
    {
        centerX = random(0, MATRIX_WIDTH);
        centerY = random(0, MATRIX_HEIGHT);
    }

    void reset()
    {
        startTime = millis();
        centerX = random(0, MATRIX_WIDTH);
        centerY = random(0, MATRIX_HEIGHT);
        hue = random(0, 255);
        offset = random(0, 60000 / bpm);
    }

    float radius()
    {
        float radius = beatsin16(30, 0, 500, offset) / 100.0;
        return radius;
    }
};

const int NUMBER_OF_CIRCLES = 20;

class PatternSMAmberRain : public LEDStripEffect
{
  private:
    Circle circles[NUMBER_OF_CIRCLES] = {};

    void drawCircle(Circle circle)
    {
        int16_t centerX = circle.centerX;
        int16_t centerY = circle.centerY;
        int hue = circle.hue;
        float radius = circle.radius();

        int16_t startX = centerX - ceil(radius);
        int16_t endX = centerX + ceil(radius);
        int16_t startY = centerY - ceil(radius);
        int16_t endY = centerY + ceil(radius);

        for (u_int16_t x = startX; x < endX; x++)
        {
            for (u_int16_t y = startY; y < endY; y++)
            {
                int16_t index = XY(x, y);
                double distance = sqrt(sq(x - centerX) + sq(y - centerY));
                if (distance > radius)
                    continue;

                uint16_t brightness;
                if (radius < 1)
                { // last pixel
                    brightness = 255.0 * radius;
                }
                else
                {
                    double percentage = distance / radius;
                    double fraction = 1.0 - percentage;
                    brightness = 255.0 * fraction;
                }

                if (g()->isValidPixel(index))
                    g()->leds[index] += CHSV(hue, 255, brightness);
            }
        }
    }

  public:
    PatternSMAmberRain() : LEDStripEffect(EFFECT_MATRIX_SMAMBERRAIN, "Color Rain")
    {
    }

    PatternSMAmberRain(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (int i = 0; i < NUMBER_OF_CIRCLES; i++)
        {
            circles[i].reset();
        }
    }

    void Draw() override
    {
        fadeAllChannelsToBlackBy(32);

        for (unsigned int i = 0; i < NUMBER_OF_CIRCLES; i++)
        {
            if (circles[i].radius() < 0.001)
            {
                circles[i].move();
            }
            drawCircle(circles[i]);
        }
    }
};
