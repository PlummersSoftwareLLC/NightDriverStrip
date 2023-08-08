#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1441-lumenjer-palette

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

// palette for the type of realistic waterfall (if the Scale slider is set to
// 100)
extern const TProgmemRGBPalette16 WaterfallColors_p FL_PROGMEM = {
    0x000000, 0x060707, 0x101110, 0x151717, 0x1C1D22, 0x242A28, 0x363B3A, 0x313634,
    0x505552, 0x6B6C70, 0x98A4A1, 0xC1C2C1, 0xCACECF, 0xCDDEDD, 0xDEDFE0, 0xB2BAB9};

class PatternSMLumenjerPalette : public LEDStripEffect
{
  private:
    const int DIMSPEED = (254U - 500U / MATRIX_WIDTH / MATRIX_HEIGHT);

    uint8_t hue;
    uint8_t Scale = 1; // 1-100 is palette. This would be good to control via web.

    // added changing the current palette (used in many effects below for the
    // Scale slider)
    const TProgmemRGBPalette16 *palette_arr[9] = {&PartyColors_p,  &OceanColors_p,     &LavaColors_p,
                                                  &HeatColors_p,   &WaterfallColors_p, &CloudColors_p,
                                                  &ForestColors_p, &RainbowColors_p,   &RainbowStripeColors_p};
    const TProgmemRGBPalette16 *curPalette = palette_arr[0];

  public:
    PatternSMLumenjerPalette() : LEDStripEffect(EFFECT_MATRIX_SMLUMENJER_PALETTE, "Lumenjer Palette")
    {
    }

    PatternSMLumenjerPalette(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        if (Scale > 50)
            curPalette = firePalettes[(uint8_t)((Scale - 50) / 50.0F *
                                                ((sizeof(firePalettes) / sizeof(TProgmemRGBPalette16 *)) - 0.01F))];
        else
            curPalette = palette_arr[(uint8_t)(Scale / 50.0F *
                                               ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01F))];
    }

    void Draw() override
    {
        int8_t dx = random8(3) ? dx : -dx;
        int8_t dy = random8(3) ? dy : -dy;
#if (MATRIX_WIDTH % 2 == 0 && MATRIX_HEIGHT % 2 == 0)
        uint8_t x = (MATRIX_WIDTH + x + dx * (bool)random8(64)) % MATRIX_WIDTH;
#else
        uint8_t x = (MATRIX_WIDTH + x + dx) % MATRIX_WIDTH;
#endif
        uint8_t y = (MATRIX_HEIGHT + y + dy) % MATRIX_HEIGHT;

        // leds[XY(x, y)] += ColorFromPalette(*curPalette, hue++);
        if (Scale == 100U)
            g()->leds[XY(x, y)] += CHSV(random8(), 255U, 255U);
        else
            g()->leds[XY(x, y)] += ColorFromPalette(*curPalette, hue++);
    }
};
