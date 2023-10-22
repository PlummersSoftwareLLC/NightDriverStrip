#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1177-picasso-3in1

class PatternSMPicasso3in1 : public LEDStripEffect
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
    // максимальное количество отслеживаемых объектов (очень влияет на
    // расход памяти)
    float trackingObjectPosX[trackingOBJECT_MAX_COUNT] {0};
    float trackingObjectPosY[trackingOBJECT_MAX_COUNT] {0};
    float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT] {0};
    float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT] {0};
    float trackingObjectShift[trackingOBJECT_MAX_COUNT] {0};
    uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT] {0};
    uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT] {0};
    bool trackingObjectIsShift[trackingOBJECT_MAX_COUNT] {0};
    static constexpr int enlargedOBJECT_MAX_COUNT = (MATRIX_WIDTH * 2);
    // максимальное количество сложных отслеживаемых объектов
    // (меньше, чем trackingOBJECT_MAX_COUNT)
    uint8_t enlargedObjectNUM; // используемое в эффекте количество объектов
    long enlargedObjectTime[enlargedOBJECT_MAX_COUNT] {0};
    int _scale{-1};

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
                trackingObjectPosX[i] = random8(MATRIX_WIDTH);
                trackingObjectPosY[i] = random8(MATRIX_HEIGHT);

                // curr->color = CHSV(random(1U, 255U), 255U, 255U);
                trackingObjectHue[i] = random8();

                trackingObjectSpeedY[i] = +((-maxSpeed / 3) + (maxSpeed * (float)random8(1, 100) / 100));
                trackingObjectSpeedY[i] += trackingObjectSpeedY[i] > 0 ? minSpeed : -minSpeed;

                trackingObjectShift[i] = +((-maxSpeed / 2) + (maxSpeed * (float)random8(1, 100) / 100));
                trackingObjectShift[i] += trackingObjectShift[i] > 0 ? minSpeed : -minSpeed;

                trackingObjectState[i] = trackingObjectHue[i];
            }
        }
        for (uint8_t i = 0; i < enlargedObjectNUM; i++)
        {
            if (reset)
            {
                trackingObjectState[i] = random8();
                trackingObjectSpeedX[i] = (trackingObjectState[i] - trackingObjectHue[i]) / 25;
            }
            if (trackingObjectState[i] != trackingObjectHue[i] && trackingObjectSpeedX[i])
            {
                trackingObjectHue[i] += trackingObjectSpeedX[i];
            }
        }
    }

    void PicassoPosition()
    {
        for (uint8_t i = 0; i < enlargedObjectNUM; i++)
        {
            if (trackingObjectPosX[i] + trackingObjectSpeedY[i] > MATRIX_WIDTH ||
                trackingObjectPosX[i] + trackingObjectSpeedY[i] < 0)
            {
                trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
            }

            if (trackingObjectPosY[i] + trackingObjectShift[i] > MATRIX_HEIGHT ||
                trackingObjectPosY[i] + trackingObjectShift[i] < 0)
            {
                trackingObjectShift[i] = -trackingObjectShift[i];
            }

            trackingObjectPosX[i] += trackingObjectSpeedY[i];
            trackingObjectPosY[i] += trackingObjectShift[i];
        };
    }

    void PicassoRoutine1()
    {
        PicassoGenerate(false);
        PicassoPosition();

        for (uint8_t i = 0; i < enlargedObjectNUM - 2U; i += 2)
        {
            g()->drawLine(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i + 1U],
                     trackingObjectPosY[i + 1U], CHSV(trackingObjectHue[i], 255U, 255U));
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
            g()->drawLine(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i + 1U],
                      trackingObjectPosY[i + 1U], CHSV(trackingObjectHue[i], 255U, 255U));

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
            g()->DrawSafeCircle(fabs(trackingObjectPosX[i] - trackingObjectPosX[i + 1U]),
                       fabs(trackingObjectPosY[i] - trackingObjectPosX[i + 1U]),
                       fabs(trackingObjectPosX[i] - trackingObjectPosY[i]), CHSV(trackingObjectHue[i], 255U, 255U));

        EVERY_N_MILLIS(20000)
        {
            PicassoGenerate(true);
        }
        g()->BlurFrame(80);
    }

  public:
    PatternSMPicasso3in1()
      : LEDStripEffect(EFFECT_MATRIX_SMPICASSO3IN1, "Picasso"),
        _scale(-1)
    {
    }

    PatternSMPicasso3in1(const String& name, int scale)
      : LEDStripEffect(EFFECT_MATRIX_SMPICASSO3IN1, name),
        _scale(scale)
    {
    }


    PatternSMPicasso3in1(const JsonObjectConst &jsonObject)
      : LEDStripEffect(jsonObject),
      _scale(jsonObject[PTY_SCALE])
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<LEDStripEffect::_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_SCALE] = _scale;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
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

        if (Scale < 34U) // если масштаб до 34
            PicassoRoutine1();
        else if (Scale > 67U) // если масштаб больше 67
            PicassoRoutine3();
        else // для масштабов посередине
            PicassoRoutine2();
    }
};
