#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1138-fire2012

#if ENABLE_AUDIO
class PatternSMFire2012 : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMFire2012 : public LEDStripEffect
#endif
{
private:
  uint8_t Scale = 63; // 0-100. < 50= FirePalette + scale > 50, choose scale // Should be a Setting.

  // TProgmemRGBPalette16 *curPalette = &PartyColors_p;
  // добавлено изменение текущей палитры (используется во многих эффектах ниже для бегунка Масштаб)
const TProgmemRGBPalette16 *palette_arr[9] = {
    &PartyColors_p,
    &OceanColors_p,
    &LavaColors_p,
    &HeatColors_p,
    &WaterfallColors_p,
    &CloudColors_p,
    &ForestColors_p,
    &RainbowColors_p,
    &RainbowStripeColors_p};
const TProgmemRGBPalette16 *curPalette = palette_arr[0];

#define NUM_LAYERSMAX 2
  uint8_t noise3d[NUM_LAYERSMAX][MATRIX_WIDTH][MATRIX_HEIGHT];     // двухслойная маска или хранилище свойств в размер всей матрицы

  static inline uint8_t wrapX(int8_t x){
    return (x + MATRIX_WIDTH) % MATRIX_WIDTH;
  }
  static inline uint8_t wrapY(int8_t y){
    return (y + MATRIX_HEIGHT) % MATRIX_HEIGHT;
  }
public:

  PatternSMFire2012() :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMFIRE2012, "Fire 2012")
    {
    }

  PatternSMFire2012(const JsonObjectConst& jsonObject) :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  virtual void Start() override
  {
    g()->Clear();

    if (Scale > 100) Scale = 100; // чтобы не было проблем при прошивке без очистки памяти
    if (Scale > 50)
      curPalette = firePalettes[(uint8_t)((Scale - 50)/50.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = palette_arr[(uint8_t)(Scale/50.0F * ((sizeof(palette_arr)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
  }

  virtual void Draw() override
  {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    #if MATRIX_HEIGHT/6 > 6
  #define FIRE_BASE 6
#else
  #define FIRE_BASE MATRIX_HEIGHT/6+1
#endif
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  uint8_t cooling = 70;
  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  uint8_t sparking = 130;
  // SMOOTHING; How much blending should be done between frames
  // Lower = more blending and smoother flames. Higher = less blending and flickery flames
  const uint8_t fireSmoothing = 80;
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Loop for each column individually
  for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < MATRIX_HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random(0, ((cooling * 10) / MATRIX_HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = MATRIX_HEIGHT - 1; k > 0; k--) { // fixed by SottNick
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 1] + noise3d[0][x][wrapY(k - 2)]) / 3; // fixed by SottNick
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < sparking) {
      uint8_t j = random8(FIRE_BASE);
      noise3d[0][x][j] = qadd8(noise3d[0][x][j], random(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    // Blend new data with previous frame. Average data between neighbouring pixels
    // Nightdriver/Mesmerizer change: invert Y axis.
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
      nblend(g()->leds[g()->xy(x,MATRIX_HEIGHT - 1 - y)], ColorFromPalette(*curPalette, ((noise3d[0][x][y]*0.7) + (noise3d[0][wrapX(x+1)][y]*0.3))), fireSmoothing);
  }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override
  {

  }
#endif
};
