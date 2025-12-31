#pragma once

#include <cinttypes>

#include "effectmanager.h"

// Inspired from
// https://editor.soulmatelights.com/gallery/1165-fireworks-by-shaitan

class PatternSMFireworks : public EffectWithId<PatternSMFireworks>
{
  private:
    //    uint8_t Scale = 50;  // 1-100 is size // Suggested setting.
    uint8_t Scale = 25; // 1-100 is size // Suggested setting. Probably should
                        // base on MATRIX_HEIGHT.

    uint8_t step = 0; // a counter of frames or sequences of operations
    uint8_t hue2;

    int SaluteX, SaluteY;
    int SaluteDX, SaluteDY;
    int Saluteerr;
    int Saluteystep;
    CRGB SaluteColor;

    uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много
    uint8_t deltaValue;          // просто повторно используемая переменная
    uint8_t poleX,
        poleY; // размер всего поля по горизонтали / вертикали (в том числе 1
               // дополнительная пустая дорожка-разделитель с какой-то из сторон)

  public:
    PatternSMFireworks() : EffectWithId<PatternSMFireworks>("Fireworks")
    {
    }

    PatternSMFireworks(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMFireworks>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void SaluteStart()
    {
        step = 1;
        SaluteColor = CHSV(random(0, 9) * 28, 255U, 255U);
        poleY = 0;
        deltaHue2 = random(MATRIX_HEIGHT * 2 / 3, MATRIX_HEIGHT);
        poleX = random(0, MATRIX_WIDTH);
        deltaHue = random(poleX, MATRIX_WIDTH);
        // deltaHue=random(0,MATRIX_WIDTH);
        hue2 = 0;

        SaluteDX = deltaHue - poleX;
        SaluteDY = deltaHue2;

        int tmp;
        /*
          if ( poleX > deltaHue )
          {
            tmp = poleX; poleX = deltaHue; deltaHue = tmp;
            tmp = poleY; poleY = deltaHue2; deltaHue2 = tmp;
          }
        */
        if (SaluteDY > SaluteDX)
        {
            hue2 = 1;
            tmp = SaluteDX;
            SaluteDX = SaluteDY;
            SaluteDY = tmp;
            tmp = poleX;
            poleX = poleY;
            poleY = tmp;
            tmp = deltaHue;
            deltaHue = deltaHue2;
            deltaHue2 = tmp;
        }
        Saluteerr = SaluteDX >> 1;
        if (deltaHue2 > poleY)
            Saluteystep = 1;
        else
            Saluteystep = -1;
        SaluteY = poleY;
        SaluteX = poleX;
    }

    void SaluteFadeAll(uint8_t val)
    {
#if 0
      fadeAllChannelsToBlackBy(val);
#else
        for (int x = 0; x < MATRIX_WIDTH; x++)
            for (int y = 0; y < MATRIX_HEIGHT; y++)
                g()->leds[XY(x, y)] -= CHSV(0, 0, val);
#endif
    }
    void SaluteDrawLine()
    {
        if (SaluteX < deltaHue)
        {
            if (hue2 == 0)
                g()->drawPixel(SaluteX, MATRIX_HEIGHT - SaluteY, SaluteColor);
            else
                g()->drawPixel(SaluteY, MATRIX_HEIGHT - SaluteX, SaluteColor);
            Saluteerr -= (uint8_t)SaluteDY;
            if (Saluteerr < 0)
            {
                SaluteY += Saluteystep;
                Saluteerr += SaluteDX;
            }
            SaluteX++;
        }
        else
        {
            step = 2;
            SaluteColor = CHSV(random(0, 9) * 28, 128U, 255U);
            deltaValue = 1;
        }
    }

    void SaluteExplosion()
    {
        if (deltaValue < ::map(Scale, 1, 100, 2, MATRIX_WIDTH * 0.7))
        {
            // X and Y are swapped. No idea why...
            if (hue2 == 0)
                g()->DrawSafeCircle(SaluteX, MATRIX_HEIGHT - SaluteY, deltaValue,
                           SaluteColor - CHSV(0, 0, deltaValue * 64) + CHSV(0, deltaValue * 32, 0));
            else
                g()->DrawSafeCircle(SaluteY, MATRIX_HEIGHT - SaluteX, deltaValue,
                           SaluteColor - CHSV(0, 0, deltaValue * 64) + CHSV(0, deltaValue * 32, 0));
            deltaValue++;
        }
        else
            step = 3;
    }

    void SaluteDecay()
    {
        if (deltaValue > 0)
            deltaValue--;
        else
            step = 0;
    }

    void Draw() override
    {
        switch (step)
        {
        case 0:
            SaluteStart();
            break;
        case 1:
            SaluteDrawLine();
            break;
        case 2:
            SaluteExplosion();
            break;
        case 3:
            SaluteDecay();
            break;
        default:
            debugI("Invalid Step %d\n", step);
            step = 0;
            break;
        }
        SaluteFadeAll(32);
    }
};
