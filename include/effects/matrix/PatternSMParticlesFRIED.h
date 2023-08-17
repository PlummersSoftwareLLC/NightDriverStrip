#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/2358-particles

class PatternSMParticles : public LEDStripEffect
{
  private:
    const int thisScale = 254; // 254 - максимальный, наверное, масштаб
    const int CUR_PRES_speed = 15; // 15 - скорость, на которой частицы пляшут туда-сюда
    const int CUR_PRES_color = 255; // 1 - цвет частиц, чтобы не заморачиваться с палитрами

    const int cfg_length = MATRIX_HEIGHT;
    const int cfg_width = MATRIX_WIDTH;
    const int now_weekMs = 0 * 1000ul + millis(); // - tmr

  public:
    PatternSMParticles() : LEDStripEffect(EFFECT_MATRIX_SMPARTICLES, "Particles")
    {
    }

    PatternSMParticles(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        ///    for (int i = 0; i < cfg_length * cfg_width; i++) {
        //      g()->leds[i].fadeToBlackBy(70);
        //    }
        //    fadeAllChannelsToBlackBy(70);
        uint16_t rndVal = 0;
        byte amount = (thisScale >> 3) + 1;
#if 0
    for (int x = 0; x < MATRIX_WIDTH; x++)
    for (int y = 0; y < MATRIX_HEIGHT; y++)
        g()->leds[XY(x, y)] = RGB(255,255,255);
#endif
        for (int y = 0; y < MATRIX_HEIGHT; y++)
            g()->leds[XY(y, y)] = CRGB(0, 0, 255);

        for (int i = 0; i < amount; i++)
        {
            rndVal = rndVal * 2053 + 13849; // random2053 алгоритм
            int homeX = inoise16(i * 100000000ul + (now_weekMs << 3) * CUR_PRES_speed / 255);
            homeX = map(homeX, 15000, 50000, 0, cfg_length);
            int offsX = inoise8(i * 2500 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
            offsX = cfg_length / 2 * offsX / 128;
            int thisX = homeX + offsX;

            int homeY = inoise16(i * 100000000ul + 2000000000ul + (now_weekMs << 3) * CUR_PRES_speed / 255);
            homeY = map(homeY, 15000, 50000, 0, cfg_width);
            int offsY = inoise8(i * 2500 + 30000 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
            offsY = cfg_length / 2 * offsY / 128;
            int thisY = homeY + offsY;
            // debugI(" %d: %d %d %d %d", i, thisX, thisY, homeX, homeY);
            if ((thisX > 0) && (thisY > 0))
            {
                // g()->leds[XY(thisX, MATRIX_HEIGHT-thisY)] =
                // CRGB(CHSV(CUR_PRES_color, 255, 255));
                g()->leds[XY(thisX, MATRIX_HEIGHT - thisY)] = CRGB(20, 255, 0);
            }
        }
    }
};
