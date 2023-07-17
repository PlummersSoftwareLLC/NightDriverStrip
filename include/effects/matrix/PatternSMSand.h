#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1127-sand
// This looks better on a square display than a landscape one, but it's
// stil cool enough to include, IMO.

#if ENABLE_AUDIO
class PatternSMSand : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMSand : public LEDStripEffect
#endif
{
 private:
   uint8_t Scale = 85;        // 1-100 limit Should be a setting. % of screen height

   static constexpr int WIDTH = MATRIX_WIDTH;
   static constexpr int HEIGHT = MATRIX_HEIGHT;

   // matrix size constants are calculated only here and do not change in effects
   static constexpr uint8_t CENTER_X_MINOR = (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01); // the center of the matrix according to ICSU, shifted to the smaller side, if the width is even
   static constexpr uint8_t CENTER_Y_MINOR = (MATRIX_HEIGHT / 2) - ((MATRIX_HEIGHT - 1) & 0x01); // center of the YGREK matrix, shifted down if the height is even
   static constexpr uint8_t CENTER_X_MAJOR = MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2); // the center of the matrix according to IKSU, shifted to a larger side, if the width is even
   static constexpr uint8_t CENTER_Y_MAJOR = MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2); // center of the YGREK matrix, shifted up if the height is even
#if ENABLE_AUDIO
   static constexpr uint8_t top_reserve = 1; // I'm not sure that every place that needs it has it.
#else
   static constexpr uint8_t top_reserve = 0;
#endif


  uint8_t pcnt{0};                                      // какой-то счётчик какого-то прогресса

public:
  PatternSMSand() :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMSAND, "Sand")
    {
    }

  PatternSMSand(const JsonObjectConst& jsonObject) :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  virtual void Start() override
  {
    g()->Clear();
  }

  CRGB getPixColorXY(uint8_t x, uint8_t y) const
  {
    // Just don't think about what this does to prefetch and prediction...
    return g()->leds[g()->xy(x, MATRIX_HEIGHT - 1 - y)];
  }

  void drawPixelXY(int8_t x, int8_t y, CRGB color) const
  {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1)) return;
    uint32_t thisPixel = g()->xy(x, MATRIX_HEIGHT - top_reserve - y);
    g()->leds[thisPixel] = color;
  } // служебные функции



  virtual void Draw() override
  {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

  // if enough has already been poured, bang random grains of sand
  uint8_t temp = map8(random8(), Scale * 2.55, 255U);
  if (pcnt >= map8(temp, 2U, HEIGHT - 3U)) {
    temp = HEIGHT + 1U - pcnt;
    if (!random8(4U)) // sometimes sand crumbles up to half at once
      if (random8(2U))
        temp = 2U;
      else
        temp = 3U;
    for (uint8_t y = 1; y < pcnt; y++)
      for (uint8_t x = 0; x < WIDTH; x++)
        if (!random8(temp))
          g()->leds[g()->xy(x,y)] = CRGB::Black;
  }

  pcnt = 0U;
  // In this effect, 0, 0 is our lower left corner.
  // Scroll down everything on the screen.
  // Note Mesmerizer/NightDriver hack here to dodge the audio line on the top of
  // the screen as scrolling that down is visually interesting, but wrong.
  for (uint8_t y = 0; y < HEIGHT - top_reserve ; y++) // Skip audio line
    for (uint8_t x = 0; x < WIDTH; x++)
      if (auto me = getPixColorXY(x,y)) // checking for every grain of sand
        if (!getPixColorXY(x,y-1)){     // if it's empty below us, we just fall
          drawPixelXY(x,y-1, me);
          drawPixelXY(x,y,CRGB::Black);
        }
        else if (x>0U && !getPixColorXY(x-1,y-1) && x<WIDTH-1 && !getPixColorXY(x+1,y-1)){   // if we have a peak
          if (random8(2U))
            drawPixelXY(x-1,y-1, me);
          else
            drawPixelXY(x-1,y-1, me);
          drawPixelXY(x,y,CRGB::Black);
          pcnt = y-1;
        }
        else if (x>0U && !getPixColorXY(x-1,y-1)){   // a a slope to the left
          drawPixelXY(x-1,y-1, me);
          drawPixelXY(x,y,CRGB::Black);
          pcnt = y-1;
        }
        else if (x<WIDTH-1 && !getPixColorXY(x+1,y-1)){ // if below us slopes to the right
          // drawPixelXY(x+1,y-1, getPixColorXY(x,y));
          drawPixelXY(x+1,y-1, me);
          drawPixelXY(x,y,CRGB::Black);
          pcnt = y-1;
        }
        else // if there is a plateau below us
          pcnt = y;

  // Emit randomly colored new grains of sand at top.
  // It's odd that the grains are "clearly" random8() colored, but visually they're always
  // drawn either green or blue and get their color while falling or while falling down
  // the pyramid. At taller heights, the apex is most likely to be blue or green. It's
  // an oddity, but it doesn't bother me enough to figure out why. (This code is kind of
  // brain-hurting anyway.)
  if (!getPixColorXY(CENTER_X_MINOR,HEIGHT-1 - top_reserve) && !getPixColorXY(CENTER_X_MAJOR,HEIGHT-1 - top_reserve) && !random8(3)) {
    temp = random8(2) ? CENTER_X_MINOR : CENTER_X_MAJOR;
    // g()->leds[g()->xy(temp,HEIGHT-1)] = CHSV(random8(), 255U, 255U);
    // HEIGHT -1 is VU bar for ambient audio.
    drawPixelXY(temp,HEIGHT-2, CHSV(random8(), 255U, 255U));
  }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {
    // Force deletion (on the next Draw()) of the bottom row. It's simple,
    // but it's enough to make the pyramid visibly respond to ambient sounds.
    for (uint8_t x = 0; x < WIDTH; x++) {
      drawPixelXY(x, 0, CRGB::Black);
    }
  }
#endif
};
