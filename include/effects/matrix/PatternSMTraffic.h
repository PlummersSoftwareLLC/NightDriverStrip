#pragma once

#include "effectmanager.h"

#include <cinttypes>

// Derived from https://editor.soulmatelights.com/gallery/1404-traffic

class PatternSMTraffic : public LEDStripEffect
{
  private:
    uint8_t center;
    uint8_t centerY;
    uint8_t divider;

    uint32_t light = 0xCFCF20;
    uint32_t atr_light = 0xCFCF10;
    uint32_t stop_light = 0x7F000F;
    uint32_t atr_stop = 0x6F0008;
    uint32_t color_force;
    uint32_t color;

    uint8_t Speed = 20;  // 1-255  - this should be a UX setting.
    uint8_t Scale = 100; // 1-100  - this should be a UX setting.

    // несколько общих переменных и буферов, которые могут использоваться в любом
    // эффекте
    uint8_t hue; // постепенный сдвиг оттенка или какой-нибудь другой цикличный
                 // счётчик
    uint8_t deltaHue; // ещё пара таких же, когда нужно много
    uint8_t step; // какой-нибудь счётчик кадров или последовательностей операций
    uint8_t pcnt;       // какой-то счётчик какого-то прогресса
    uint8_t deltaValue; // просто повторно используемая переменная

    static constexpr uint32_t colors[4][4] = {// light |atr_light |stop_light|atr_stop
                                              {0xCFCF20, 0xCFCF10, 0x7F000F, 0x6F0008},
                                              {0x5F5F5F, 0x5F5F3F, 0x00707F, 0x006060},
                                              {0x306F00, 0x306F05, 0x4F004F, 0x4F054F},
                                              {0xFFFFFF, 0xFFFFF5, 0xF0E68C, 0xF0E685}};

  public:
    PatternSMTraffic() : LEDStripEffect(EFFECT_MATRIX_SMTRAFFIC, "Traffic")
    {
    }

    PatternSMTraffic(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        deltaValue = 255U - Speed + 1U;
        step = deltaValue; // чтообы при старте эффекта сразу покрасить лампу
        hue = 0;
        center = floor(MATRIX_WIDTH * 0.5);
        centerY = floor(MATRIX_HEIGHT * 0.5);
        drawTrafficLight(hue, false);
    }

    // функция получения цвета пикселя по его номеру
    CRGB getPixColor(uint32_t thisSegm) const
    {
        uint32_t thisPixel = thisSegm; // * SEGMENTS;
        if (thisPixel > NUM_LEDS - 1)
            return 0;
        return g()->leds[thisPixel];
    }

    // функция получения цвета пикселя в матрице по его координатам
    [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const
    {
        // Just don't think about what this does to prefetch and prediction...
        return g()->leds[XY(x, y)];
    }

    // функция отрисовки точки по координатам X Y
    void drawPixelXY(int8_t x, int8_t y, CRGB color)
    {
		if (!g()->isValidPixel(x, y))
            return;
        uint32_t thisPixel = XY((uint8_t)x, (uint8_t)y); // * SEGMENTS;
        // for (uint8_t i = 0; i < SEGMENTS; i++)
        //{
        g()->leds[thisPixel] = color;
        //}
    } // служебные функции

    // ============== Traffic ================
    //      © SlingMaster | by Alex Dovby
    //              EFF_TRAFFIC
    //                Traffic
    //---------------------------------------
    void drawTrafficLight(uint8_t sts, boolean swap_color)
    {
        uint8_t center = floor(MATRIX_WIDTH * 0.5);
        uint8_t centerY = floor(MATRIX_HEIGHT * 0.5);
        uint32_t light_colorV = sts == 1 ? CRGB::Green : CRGB::Red;
        uint32_t light_colorH = sts == 0 ? CRGB::Green : CRGB::Red;

        if (swap_color)
        {
            light_colorV = CRGB::Yellow;
            light_colorH = CRGB::Yellow;
        }

        drawPixelXY(center + 4, centerY + 4, light_colorV);
        drawPixelXY(center + 5, centerY + 5, light_colorV);
        drawPixelXY(center - 5, centerY - 5, light_colorH);
        drawPixelXY(center - 5, centerY - 5, light_colorH);

        drawPixelXY(center, centerY + 1, CRGB::Black);
        drawPixelXY(center, centerY - 1, CRGB::Black);
        drawPixelXY(center - 1, centerY, CRGB::Black);
        drawPixelXY(center + 1, centerY, CRGB::Black);

        if (sts == 1)
        {
            drawPixelXY(center + 2, centerY + 1, CRGB::Black);
            drawPixelXY(center - 2, centerY - 1, CRGB::Black);
        }
        else
        {
            drawPixelXY(center - 1, centerY + 2, CRGB::Black);
            drawPixelXY(center + 1, centerY - 2, CRGB::Black);
        }
    }

    void Draw() override
    {
        if (step >= deltaValue)
        {
            step = 0U;
        }
        if (step % 120 == 0)
        {
            drawTrafficLight(hue, true);
        }

        if (step % 128 == 0)
        {
            hue++;
            if (hue > 1)
            {
                hue = 0;
            }
            drawTrafficLight(hue, false);
        }

        divider = floor((Scale - 1) / 25); // маштаб задает альтернативный цвет
        light = colors[divider][0];
        atr_light = colors[divider][1];
        stop_light = colors[divider][2];
        atr_stop = colors[divider][3];

        //  drawPixelXY(center, centerY, 0x747474);

        if (hue == 1)
        {
            // Green Traffic -------
            // [B] ****
            color_force = (divider == 0) ? CRGB::Blue : atr_stop;
            color = ((random8(2)) == 1   ? CRGB::Black
                     : (random8(2)) == 1 ? (random8(20) == 5 ? color_force : atr_stop)
                                         : stop_light);
            drawPixelXY((center + 1), 0, color);

            // [T] ****
            color_force = (divider == 0) ? CRGB::Blue : atr_light;
            color = ((random8(2)) == 1   ? CRGB::Black
                     : (random8(2)) == 1 ? (random8(20) == 5 ? color_force : atr_light)
                                         : light);
            drawPixelXY((center - 1), MATRIX_HEIGHT - 1, color);

            // [B] ****
            for (uint8_t y = centerY - 2; y > 0; y--)
            {
                drawPixelXY((center + 1), y, getPixColorXY((center + 1U), y - 1U));
            }
            if (getPixColorXY((center + 1U), centerY - 2) == atr_stop)
            {
                drawPixelXY((center + 1), centerY - 1, light);
            }
            else
            {
                drawPixelXY((center + 1), centerY, getPixColorXY((center + 1U), centerY - 2));
            }

            // [T] ****
            for (uint8_t y = centerY + 2; y < MATRIX_HEIGHT - 1; y++)
            {
                drawPixelXY((center - 1), y, getPixColorXY((center - 1U), y + 1U));
            }
            if (getPixColorXY((center - 1U), centerY + 2) == atr_light)
            {
                drawPixelXY((center - 1), centerY + 1, stop_light);
            }
            else
            {
                drawPixelXY((center - 1), centerY, getPixColorXY((center - 1U), centerY + 2));
            }
        }
        else
        {
            // Red Traffic ---------
            // [R] ****
            color = ((random8(2)) == 1 ? CRGB::Black : (random8(2)) == 1 ? atr_stop : stop_light);
            drawPixelXY((MATRIX_WIDTH - 1), centerY + 1, color);
            // [L] ****
            color = ((random8(2)) == 1   ? CRGB::Black
                     : (random8(2)) == 1 ? (random8(20) == 5 ? CRGB::Blue : atr_light)
                                         : light);
            drawPixelXY(0, centerY - 1, color);

            // сдвигаем поток влево ---------- [TL] <==
            for (uint8_t x = center - 2; x < (MATRIX_WIDTH - 1); x++)
            {
                drawPixelXY(x, centerY + 1, getPixColorXY(x + 1, centerY + 1));
            }
            if (getPixColorXY((center + 2U), centerY + 1) == atr_light)
            {
                drawPixelXY((center + 1), centerY + 1, light);
            }
            else
            {
                drawPixelXY((center), centerY + 1, getPixColorXY((center + 2U), centerY + 1));
            }

            // сдвигаем поток вправо ---------- [BR] ==>
            for (uint8_t x = center - 2; x > 0; x--)
            {
                drawPixelXY(x, centerY - 1U, getPixColorXY(x - 1U, centerY - 1U));
            }
            if (getPixColorXY((center - 2U), centerY - 1) == atr_light)
            {
                drawPixelXY((center - 1), centerY - 1, light);
            }
            else
            {
                drawPixelXY((center), centerY - 1, getPixColorXY((center - 2U), centerY - 1));
            }
        }

        // *********
        // сдвигаем поток вверх -----------
        for (uint8_t y = MATRIX_HEIGHT - 1; y > centerY; y--)
        {
            drawPixelXY((center + 1), y, getPixColorXY((center + 1U), y - 1U));
        }
        // сдвигаем поток вниз -----------
        for (uint8_t y = 0U; y < centerY; y++)
        {
            drawPixelXY((center - 1), y, getPixColorXY((center - 1U), y + 1U));
        }
        // сдвигаем поток вправо ---------- [BR] ==>
        for (uint8_t x = MATRIX_WIDTH - 1; x > center; x--)
        {
            drawPixelXY(x, centerY - 1U, getPixColorXY(x - 1U, centerY - 1U));
        }
        // сдвигаем поток влево ---------- [TL] <==
        for (uint8_t x = 0U; x < center; x++)
        {
            drawPixelXY(x, centerY + 1, getPixColorXY(x + 1, centerY + 1));
        }
        // *********
        // LOG.printf_P(PSTR("Trafic | hue = %03d | divider = %01d | step =
        // %03d\n\r"), hue, divider,  step);
        // -------------------------------------
        step++;
    }
};
