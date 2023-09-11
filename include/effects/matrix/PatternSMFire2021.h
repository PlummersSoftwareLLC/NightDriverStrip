#pragma once

#include "effectmanager.h"

// additional palettes
// converted from 4 colors to 16 for writing in PROGMEM at
// https://colordesigner.io/gradient-generator, but not sure if this is
// equivalent to CRGBPalette16() values of color constants here:
// https://github.com/FastLED/FastLED/wiki/Pixel-reference
extern const TProgmemRGBPalette16 WoodFireColors_p FL_PROGMEM = {
    CRGB::Black, 0x330e00, 0x661c00,     0x992900, 0xcc3700, CRGB::OrangeRed, 0xff5800, 0xff6b00,
    0xff7f00,    0xff9200, CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300,        0xffcd00, CRGB::Gold}; //* Orange
extern const TProgmemRGBPalette16 NormalFire_p FL_PROGMEM = {
    CRGB::Black, 0x330000, 0x660000, 0x990000, 0xcc0000, CRGB::Red, 0xff0c00, 0xff1800, 0xff2400,
    0xff3000,    0xff3c00, 0xff4800, 0xff5400, 0xff6000, 0xff6c00,  0xff7800}; // пытаюсь сделать что-то более приличное
extern const TProgmemRGBPalette16 NormalFire2_p FL_PROGMEM = {CRGB::Black,     0x560000, 0x6b0000, 0x820000, 0x9a0011,
                                                              CRGB::FireBrick, 0xc22520, 0xd12a1c, 0xe12f17, 0xf0350f,
                                                              0xff3c00,        0xff6400, 0xff8300, 0xffa000, 0xffba00,
                                                              0xffd400}; // пытаюсь сделать что-то более приличное
extern const TProgmemRGBPalette16 LithiumFireColors_p FL_PROGMEM = {
    CRGB::Black, 0x240707, 0x470e0e,   0x6b1414, 0x8e1b1b, CRGB::FireBrick, 0xc14244, 0xd16166,
    0xe08187,    0xf0a0a9, CRGB::Pink, 0xff9ec0, 0xff7bb5, 0xff59a9,        0xff369e, CRGB::DeepPink}; //* Red
extern const TProgmemRGBPalette16 SodiumFireColors_p FL_PROGMEM = {
    CRGB::Black, 0x332100, 0x664200,   0x996300, 0xcc8400, CRGB::Orange, 0xffaf00, 0xffb900,
    0xffc300,    0xffcd00, CRGB::Gold, 0xf8cd06, 0xf0c30d, 0xe9b913,     0xe1af1a, CRGB::Goldenrod}; //* Yellow
extern const TProgmemRGBPalette16 CopperFireColors_p FL_PROGMEM = {
    CRGB::Black, 0x001a00, 0x003300,          0x004d00, 0x006600, CRGB::Green, 0x239909, 0x45b313,
    0x68cc1c,    0x8ae626, CRGB::GreenYellow, 0x94f530, 0x7ceb30, 0x63e131,    0x4bd731, CRGB::LimeGreen}; //* Green
extern const TProgmemRGBPalette16 AlcoholFireColors_p FL_PROGMEM = {
    CRGB::Black, 0x000033, 0x000066,          0x000099, 0x0000cc, CRGB::Blue, 0x0026ff, 0x004cff,
    0x0073ff,    0x0099ff, CRGB::DeepSkyBlue, 0x1bc2fe, 0x36c5fd, 0x51c8fc,   0x6ccbfb, CRGB::LightSkyBlue}; //* Blue
extern const TProgmemRGBPalette16 RubidiumFireColors_p FL_PROGMEM = {
    CRGB::Black,  0x0f001a,     0x1e0034, 0x2d004e, 0x3c0068, CRGB::Indigo, CRGB::Indigo,  CRGB::Indigo, CRGB::Indigo,
    CRGB::Indigo, CRGB::Indigo, 0x3c0084, 0x2d0086, 0x1e0087, 0x0f0089,     CRGB::DarkBlue}; //* Indigo
extern const TProgmemRGBPalette16 PotassiumFireColors_p FL_PROGMEM = {
    CRGB::Black, 0x0f001a, 0x1e0034,           0x2d004e, 0x3c0068, CRGB::Indigo, 0x591694, 0x682da6,
    0x7643b7,    0x855ac9, CRGB::MediumPurple, 0xa95ecd, 0xbe4bbe, 0xd439b0,     0xe926a1, CRGB::DeepPink}; //* Violet
const TProgmemRGBPalette16 *firePalettes[] = {&HeatColors_p, // // this palette is already in the main set. if both sets
                                                             // of palettes are connected in the effect, then a copy is
                                                             // not needed
                                              &WoodFireColors_p, &NormalFire_p, &NormalFire2_p, &LithiumFireColors_p,
                                              &SodiumFireColors_p, &CopperFireColors_p, &AlcoholFireColors_p,
                                              &RubidiumFireColors_p, &PotassiumFireColors_p};

// Derived from https://editor.soulmatelights.com/gallery/388-fire2021

class PatternSMFire2021 : public LEDStripEffect
{
  private:
    uint8_t Speed = 150; // 1-252 ...why is not 255?! // Setting
    uint8_t Scale = 9;   // 1-99 is palette and scale // Setting

    uint8_t pcnt;              // какой-то счётчик какого-то прогресса
    uint8_t deltaValue;        // просто повторно используемая переменная
    uint16_t ff_x, ff_y, ff_z; // большие счётчики
    uint8_t step; // какой-нибудь счётчик кадров или последовательностей операций

    const TProgmemRGBPalette16 *curPalette;

  public:
    PatternSMFire2021() : LEDStripEffect(EFFECT_MATRIX_SMFIRE2021, "Fireplace")
    {
    }

    PatternSMFire2021(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        if (Scale > 100U)
            Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
        deltaValue = Scale * 0.0899; // /100.0F * ((sizeof(palette_arr)
                                     // /sizeof(TProgmemRGBPalette16 *))-0.01F));
#if LATER
        if (deltaValue == 3U || deltaValue == 4U)
            curPalette = palette_arr[deltaValue]; // (uint8_t)(Scale/100.0F * ((sizeof(palette_arr)
                                                  // /sizeof(TProgmemRGBPalette16 *))-0.01F))];
        else
#endif
            curPalette = firePalettes[deltaValue]; // (uint8_t)(Scale/100.0F *
                                                   // ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16
                                                   // *))-0.01F))];
        deltaValue = (((Scale - 1U) % 11U + 1U));
        step = map(Speed * Speed, 1U, 65025U, (deltaValue - 1U) / 2U + 1U,
                   deltaValue * 18U + 44); // корректируем скорость эффекта в наш диапазон допустимых
        // deltaValue = (((Scale - 1U) % 11U + 2U) << 4U); // ширина языков пламени
        // (масштаб шума Перлина)
        deltaValue = 0.7 * deltaValue * deltaValue + 31.3; // ширина языков пламени (масштаб шума Перлина)
        pcnt = map(step, 1U, 255U, 20U, 128U); // nblend 3th param
    }

    void Draw() override
    {
        ff_x += step; // static uint32_t t += speed;
        for (unsigned x = 0; x < MATRIX_WIDTH; x++)
        {
            for (unsigned y = 0; y < MATRIX_HEIGHT; y++)
            {
                int16_t Bri = inoise8(x * deltaValue, (y * deltaValue) - ff_x, ff_z) - (y * (255 / MATRIX_HEIGHT));
                byte Col = Bri; // inoise8(x * deltaValue, (y * deltaValue) - ff_x,
                                // ff_z) - (y * (255 / MATRIX_HEIGHT));
                if (Bri < 0)
                    Bri = 0;
                if (Bri != 0)
                    Bri = 256 - (Bri * 0.2);
                // NightDriver mod - invert Y argument.
                nblend(g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)], ColorFromPalette(*curPalette, Col, Bri), pcnt);
            }
        }

        if (!random8())
            ff_z++;
    }
};
