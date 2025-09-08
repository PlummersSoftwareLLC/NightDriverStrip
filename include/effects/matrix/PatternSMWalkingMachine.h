#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/1990-walking-machine

class PatternSMWalkingMachine :
    public EffectWithId<PatternSMWalkingMachine>
{
private:
    // Walking machine
    // St3p40 aka Stepko
    // 11.04.23
    // Not for small matrices
    // Second name is "dreams in night"

    struct
    {
        float posX;
        float posY;
    } dot[7];

public:
    PatternSMWalkingMachine() :
        EffectWithId<PatternSMWalkingMachine>("Machine")
    {
    }
    PatternSMWalkingMachine(const JsonObjectConst &jsonObject) :
        EffectWithId<PatternSMWalkingMachine>(jsonObject)
    {
    }

    virtual bool RequiresDoubleBuffering() const
    {
        return false; // Clears every frame, so doesn't need the old
                      // buffer copy
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        g()->Clear();

        for (uint8_t i = 0; i < 7; i++)
        {
            dot[i].posX =
                (beatsin16(4, (MATRIX_WIDTH >> 3) << 8,
                           (MATRIX_WIDTH - (MATRIX_WIDTH >> 3) - 1) << 8,
                           i * 8192, i * 8192)) /
                255.f;
            dot[i].posY =
                (beatsin16(4, (MATRIX_HEIGHT >> 3) << 8,
                           (MATRIX_HEIGHT - (MATRIX_HEIGHT >> 3) - 1)
                               << 8,
                           i * 4096, 16384 + i * 8192)) /
                255.f;
        }
        for (uint8_t i = 0; i < 7; i++)
        {
            g()->drawSafeFilledCircleF(dot[i].posX, dot[i].posY, 4,
                                       CHSV(i * 32, 255, 255));
            g()->drawLineF(dot[i].posX, dot[i].posY,
                           dot[(i + 1) % 7].posX, dot[(i + 1) % 7].posY,
                           CHSV(i * 32, 255, 255),
                           CHSV(((i + 1) % 7) * 32, 255, 255));
        }
    }
};
