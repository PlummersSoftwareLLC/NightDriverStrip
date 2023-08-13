#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1089-distorsion-waves [sic]
// Funky math, but a nice effect.

class PatternSMPastelFlutter : public LEDStripEffect
{
  private:
    const uint8_t exp_gamma[256] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,
        3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,
        9,   9,   10,  10,  10,  11,  11,  12,  12,  12,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  18,
        18,  19,  19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,  26,  26,  27,  28,  28,  29,  30,  30,  31,
        32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  44,  44,  45,  46,  47,  48,  49,
        50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  70,  71,  72,
        73,  74,  75,  77,  78,  79,  80,  82,  83,  84,  85,  87,  89,  91,  92,  93,  95,  96,  98,  99,  100, 101,
        102, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 125, 126, 128, 130, 131, 133, 135, 136,
        138, 140, 142, 143, 145, 147, 149, 151, 152, 154, 156, 158, 160, 162, 164, 165, 167, 169, 171, 173, 175, 177,
        179, 181, 183, 185, 187, 190, 192, 194, 196, 198, 200, 202, 204, 207, 209, 211, 213, 216, 218, 220, 222, 225,
        227, 229, 232, 234, 236, 239, 241, 244, 246, 249, 251, 253, 254, 255};

    const uint8_t cos_wave[256] = {
        0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   6,   6,   8,   9,   10,  11,  12,  14,  15,  17,
        18,  20,  22,  23,  25,  27,  29,  31,  33,  35,  38,  40,  42,  45,  47,  49,  52,  54,  57,  60,  62,  65,
        68,  71,  73,  76,  79,  82,  85,  88,  91,  94,  97,  100, 103, 106, 109, 113, 116, 119, 122, 125, 128, 131,
        135, 138, 141, 144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 189, 191, 194, 197,
        199, 202, 204, 207, 209, 212, 214, 216, 218, 221, 223, 225, 227, 229, 231, 232, 234, 236, 238, 239, 241, 242,
        243, 245, 246, 247, 248, 249, 250, 251, 252, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 254, 253, 253, 252, 252, 251, 250, 249, 248, 247, 246, 245, 243, 242, 241, 239, 238, 236, 234, 232, 231,
        229, 227, 225, 223, 221, 218, 216, 214, 212, 209, 207, 204, 202, 199, 197, 194, 191, 189, 186, 183, 180, 177,
        174, 171, 168, 165, 162, 159, 156, 153, 150, 147, 144, 141, 138, 135, 131, 128, 125, 122, 119, 116, 113, 109,
        106, 103, 100, 97,  94,  91,  88,  85,  82,  79,  76,  73,  71,  68,  65,  62,  60,  57,  54,  52,  49,  47,
        45,  42,  40,  38,  35,  33,  31,  29,  27,  25,  23,  22,  20,  18,  17,  15,  14,  12,  11,  10,  9,   8,
        6,   6,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,   0};

  public:
    PatternSMPastelFlutter() : LEDStripEffect(EFFECT_MATRIX_SMPASTEL_FLUTTER, "Pastel Flutter")
    {
    }

    PatternSMPastelFlutter(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        byte speed = 5;

        uint8_t w = 2;
        uint8_t scale = 4;

        uint16_t a = millis() / 32;
        uint16_t a2 = a / 2;
        uint16_t a3 = a / 3;

        uint16_t cx = beatsin8(10 - speed, 0, MATRIX_WIDTH) * scale;
        uint16_t cy = beatsin8(12 - speed, 0, MATRIX_HEIGHT) * scale;
        uint16_t cx1 = beatsin8(13 - speed, 0, MATRIX_WIDTH) * scale;
        uint16_t cy1 = beatsin8(15 - speed, 0, MATRIX_HEIGHT) * scale;
        uint16_t cx2 = beatsin8(17 - speed, 0, MATRIX_WIDTH) * scale;
        uint16_t cy2 = beatsin8(14 - speed, 0, MATRIX_HEIGHT) * scale;

        uint16_t xoffs = 0;

        for (int x = 0; x < MATRIX_WIDTH; x++)
        {

            xoffs += scale;
            uint16_t yoffs = 0;

            for (int y = 0; y < MATRIX_HEIGHT; y++)
            {

                yoffs += scale;

                // byte rdistort = cos_wave [((x+y)*8+a2)&255]>>1;
                // byte gdistort = cos_wave [((x+y)*8+a3+32)&255]>>1;
                // byte bdistort = cos_wave [((x+y)*8+a+64)&255]>>1;

                byte rdistort =
                    cos_wave[(cos_wave[((x << 3) + a) & 255] + cos_wave[((y << 3) - a2) & 255] + a3) & 255] >> 1;
                byte gdistort =
                    cos_wave[(cos_wave[((x << 3) - a2) & 255] + cos_wave[((y << 3) + a3) & 255] + a + 32) & 255] >> 1;
                byte bdistort =
                    cos_wave[(cos_wave[((x << 3) + a3) & 255] + cos_wave[((y << 3) - a) & 255] + a2 + 64) & 255] >> 1;

                byte valueR = rdistort + w * (a - (((xoffs - cx) * (xoffs - cx) + (yoffs - cy) * (yoffs - cy)) >> 7));
                byte valueG =
                    gdistort + w * (a2 - (((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1)) >> 7));
                byte valueB =
                    bdistort + w * (a3 - (((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2)) >> 7));

                valueR = cos_wave[(valueR)];
                valueG = cos_wave[(valueG)];
                valueB = cos_wave[(valueB)];

                uint16_t index = XY(x, y);
                g()->leds[index].setRGB(exp_gamma[valueR], exp_gamma[valueG], exp_gamma[valueB]);
            }
        }
    }
};
