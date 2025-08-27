#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1177-picasso-3in1

struct TrackingObject {
    float posX;
    float posY;
    float speedX;
    float speedY;
    float shift;
    uint8_t hue;
    uint8_t state;
    bool isShift;
};

class PatternSMPicasso3in1 : public EffectWithId<PatternSMPicasso3in1>
{
  private:

    // Suggested values for Mesmerizer w/ 1/2 HUB75 panel: 10, 36, 70
    uint8_t Scale = 2; // 1-100 is image type and count. THis should be a Setting 0-33 = P1,
                       // 34-68 = P2, 68-99 = Picasso3 P1 - Scale is number of independent
                       // lines drawn w/ trailers. Safe=1-8 20 or so = Pink Floyd 'Pulse'
                       // laser show. P2 - 34 & up. Scale -33 == number of vertices on a
                       // connected polyline "wire" rotating in 3d. 38 up? SLOW! P3 - 68 &
                       // up. Scale -68 -2 == number of circles  *68=2, 69=3, 70=4, etc. 80
                       // up? SLOW
    static constexpr int trackingOBJECT_MAX_COUNT = 100U;
    // maximum number of tracked objects (greatly affects memory consumption)
    TrackingObject trackingObjects[trackingOBJECT_MAX_COUNT];
    static constexpr int enlargedOBJECT_MAX_COUNT = (MATRIX_WIDTH * 2);
    // maximum number of complex tracked objects
    // (less than trackingOBJECT_MAX_COUNT)
    uint8_t enlargedObjectNUM; // используемое в эффекте количество объектов
    long enlargedObjectTime[enlargedOBJECT_MAX_COUNT] {0};
    int _scale {-1};

    void PicassoGenerate(bool reset)
    {
        static int loadingFlag = true;
        if (reset || loadingFlag)
        {
            loadingFlag = false;
            if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT)
                enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
            if (enlargedObjectNUM < 2U)
                enlargedObjectNUM = 2U;

            float minSpeed = 0.2, maxSpeed = 0.8;

            for (uint8_t i = 0; i < enlargedObjectNUM; i++)
            {
                trackingObjects[i].posX = random8(MATRIX_WIDTH);
                trackingObjects[i].posY = random8(MATRIX_HEIGHT);

                // curr->color = CHSV(random(1U, 255U), 255U, 255U);
                trackingObjects[i].hue = random8();

                trackingObjects[i].speedY = +((-maxSpeed / 3) + (maxSpeed * (float)random8(1, 100) / 100));
                trackingObjects[i].speedY += trackingObjects[i].speedY > 0 ? minSpeed : -minSpeed;

                trackingObjects[i].shift = +((-maxSpeed / 2) + (maxSpeed * (float)random8(1, 100) / 100));
                trackingObjects[i].shift += trackingObjects[i].shift > 0 ? minSpeed : -minSpeed;

                trackingObjects[i].state = trackingObjects[i].hue;
            }
        }
        for (uint8_t i = 0; i < enlargedObjectNUM; i++)
        {
            if (reset)
            {
                trackingObjects[i].state = random8();
                trackingObjects[i].speedX = (trackingObjects[i].state - trackingObjects[i].hue) / 25;
            }
            if (trackingObjects[i].state != trackingObjects[i].hue && trackingObjects[i].speedX)
            {
                trackingObjects[i].hue += trackingObjects[i].speedX;
            }
        }
    }

    void PicassoPosition()
    {
        for (uint8_t i = 0; i < enlargedObjectNUM; i++)
        {
            if (trackingObjects[i].posX + trackingObjects[i].speedY > MATRIX_WIDTH ||
                trackingObjects[i].posX + trackingObjects[i].speedY < 0)
            {
                trackingObjects[i].speedY = -trackingObjects[i].speedY;
            }

            if (trackingObjects[i].posY + trackingObjects[i].shift > MATRIX_HEIGHT ||
                trackingObjects[i].posY + trackingObjects[i].shift < 0)
            {
                trackingObjects[i].shift = -trackingObjects[i].shift;
            }

            trackingObjects[i].posX += trackingObjects[i].speedY;
            trackingObjects[i].posY += trackingObjects[i].shift;
        };
    }

    void PicassoRoutine1()
    {
        PicassoGenerate(false);
        PicassoPosition();

        for (uint8_t i = 0; i < enlargedObjectNUM - 2U; i += 2)
        {
            g()->drawLine(trackingObjects[i].posX, trackingObjects[i].posY, trackingObjects[i + 1U].posX,
                     trackingObjects[i + 1U].posY, CHSV(trackingObjects[i].hue, 255U, 255U));
            // DrawLine(trackingObjectPosX[i], trackingObjectPosY[i],
            // trackingObjectPosX[i+1U], trackingObjectPosY[i+1U],
            // ColorFromPalette(*curPalette, trackingObjectHue[i]));
        }

        g()->BlurFrame(80);
    }

    void PicassoRoutine2()
    {
        PicassoGenerate(false);
        PicassoPosition();

        g()->DimAll(180);

        for (uint8_t i = 0; i < enlargedObjectNUM - 1U; i++)
            g()->drawLine(trackingObjects[i].posX, trackingObjects[i].posY, trackingObjects[i + 1U].posX,
                      trackingObjects[i + 1U].posY, CHSV(trackingObjects[i].hue, 255U, 255U));

        EVERY_N_MILLIS(20000)
        {
            PicassoGenerate(true);
        }
        g()->BlurFrame(80);
    }

    void PicassoRoutine3()
    {
        PicassoGenerate(false);
        PicassoPosition();
        g()->DimAll(180);

        for (uint8_t i = 0; i < enlargedObjectNUM - 2U; i += 2)
            g()->DrawSafeCircle(fabs(trackingObjects[i].posX - trackingObjects[i + 1U].posX),
                       fabs(trackingObjects[i].posY - trackingObjects[i + 1U].posX),
                       fabs(trackingObjects[i].posX - trackingObjects[i].posY), CHSV(trackingObjects[i].hue, 255U, 255U));

        EVERY_N_MILLIS(20000)
        {
            PicassoGenerate(true);
        }
        g()->BlurFrame(80);
    }

  public:

    PatternSMPicasso3in1()
      : EffectWithId<PatternSMPicasso3in1>("Picasso"),
        _scale(-1)
    {
    }

    PatternSMPicasso3in1(const String& name, int scale)
      : EffectWithId<PatternSMPicasso3in1>(name),
        _scale(scale)
    {
    }


    PatternSMPicasso3in1(const JsonObjectConst &jsonObject)
      : EffectWithId<PatternSMPicasso3in1>(jsonObject),
      _scale(jsonObject[PTY_SCALE])
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_SCALE] = _scale;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }


    // This has atomicity issues. It looks at a global (boooo) that
    // may change the contents of the underlying enlarge/trackingFOO
    // arrays while they're being traversed. It's just a bad diea
    // to modify Scale while these are running, so we try to do it
    // only when Scale changes.

    void RecalibrateDrawnObjects()
    {
        if (Scale < 34U) // если масштаб до 34
            enlargedObjectNUM = (Scale - 1U) / 32.0 * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
        else if (Scale >= 68U) // если масштаб больше 67
            enlargedObjectNUM = (Scale - 68U) / 32.0 * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
        else // для масштабов посередине
            enlargedObjectNUM = (Scale - 34U) / 33.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    }

    void Start() override
    {
        g()->Clear();
	    Scale = _scale;
        RecalibrateDrawnObjects();
    }

    void Draw() override
    {
        static int last_scale = _scale;

        // Force a recalibration on effect change.
        if (last_scale != Scale)
        {
            debugA("Recalibrating from %d to %d", last_scale, Scale);
                PicassoGenerate(true);
                last_scale = Scale;
	    }

	    Scale = _scale;

        if (Scale < 34U)
            PicassoRoutine1();  // Scale is less than 34
        else if (Scale > 67U)
            PicassoRoutine3();  // Scale is greater than 67
        else
            PicassoRoutine2();  // Scale is between 34 and 67
    }
};
