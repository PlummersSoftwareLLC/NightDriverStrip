#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Inspired by https://editor.soulmatelights.com/gallery/1129-magma

#if ENABLE_AUDIO
class PatternSMMagma : public BeatEffectBase,
                       public LEDStripEffect
#else
class PatternSMMagma : public LEDStripEffect
#endif
{
 private:
  uint8_t Scale = 20;  // 1-100 is palette and count  // THIS is a setting 0-33,
                       // 34-66, 67-99

#define trackingOBJECT_MAX_COUNT \
  (100U)  // максимальное количество отслеживаемых объектов (очень влияет на
          // расход памяти)
  float trackingObjectPosX[trackingOBJECT_MAX_COUNT];
  float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
  float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
  float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
  float trackingObjectShift[trackingOBJECT_MAX_COUNT];
  uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];
  uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];
  bool trackingObjectIsShift[trackingOBJECT_MAX_COUNT];
#define enlargedOBJECT_MAX_COUNT \
  (MATRIX_WIDTH * 2)  // максимальное количество сложных отслеживаемых объектов
                      // (меньше, чем trackingOBJECT_MAX_COUNT)
  uint8_t enlargedObjectNUM;  // используемое в эффекте количество объектов
  long enlargedObjectTime[enlargedOBJECT_MAX_COUNT];

  uint8_t deltaHue, deltaHue2;  // ещё пара таких же, когда нужно много
  uint8_t deltaValue;  // просто повторно используемая переменная

  uint16_t ff_x, ff_y, ff_z;  // большие счётчики
  const TProgmemRGBPalette16* curPalette = &PartyColors_p;

  static constexpr int NUM_LAYERSMAX = 2;
  uint8_t noise3d[NUM_LAYERSMAX][MATRIX_WIDTH]
                 [MATRIX_HEIGHT];  // двухслойная маска или хранилище свойств в
                                   // размер всей матрицы
  uint8_t line[MATRIX_WIDTH];  // свойство пикселей в размер строки матрицы
  uint8_t
      shiftHue[MATRIX_HEIGHT];  // свойство пикселей в размер столбца матрицы
  uint8_t shiftValue[MATRIX_HEIGHT];

  // const int WIDTH = MATRIX_WIDTH;
  // const int HEIGHT = MATRIX_HEIGHT;

 public:
  PatternSMMagma()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMMAGMA, "Magma") {
  }

  PatternSMMagma(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();

    deltaValue = Scale * 0.0899;  // /100.0F * ((sizeof(palette_arr)
                                  // /sizeof(TProgmemRGBPalette16 *))-0.01F));
                                  //    if (deltaValue == 3U ||deltaValue == 4U)
    //      curPalette =  palette_arr[deltaValue]; // (uint8_t)(Scale/100.0F *
    //      ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F))];
    //    else
    curPalette = firePalettes[deltaValue];  // (uint8_t)(Scale/100.0F *
                                            // ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16
                                            // *))-0.01F))];
    // deltaValue = (((Scale - 1U) % 11U + 1U) << 4U) - 8U; // ширина языков
    // пламени (масштаб шума Перлина)
    deltaValue = 12U;
    deltaHue = 10U;  // map(deltaValue, 8U, 168U, 8U, 84U); // высота языков
                     // пламени должна уменьшаться не так быстро, как ширина
    // step = map(255U-deltaValue, 87U, 247U, 4U, 32U); // вероятность смещения
    // искорки по оси ИКС
    for (uint8_t j = 0; j < MATRIX_HEIGHT; j++) {
      shiftHue[j] = (MATRIX_HEIGHT - 1 - j) * 255 /
                    (MATRIX_HEIGHT - 1);  // init colorfade table
    }

    // FastLED.clear();
    // enlargedObjectNUM = (Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U)
    // + 1U;
    enlargedObjectNUM =
        (Scale - 1U) % 11U / 10.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT)
      enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    // if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      trackingObjectPosX[i] = random8(MATRIX_WIDTH);
      trackingObjectPosY[i] = random8(MATRIX_HEIGHT);

      // curr->color = CHSV(random(1U, 255U), 255U, 255U);
      trackingObjectHue[i] = 50U;
      random8();
    }
  }

  void LeapersRestart_leaper(uint8_t l) {
    // leap up and to the side with some random component
    trackingObjectSpeedX[l] = (1 * (float)random8(1, 100) / 100);
    trackingObjectSpeedY[l] = (2 * (float)random8(1, 100) / 100);

    // for variety, sometimes go 50% faster
    if (random8() < 12) {
      trackingObjectSpeedX[l] += trackingObjectSpeedX[l] * 0.5;
      trackingObjectSpeedY[l] += trackingObjectSpeedY[l] * 0.5;
    }

    // leap towards the centre of the screen
    if (trackingObjectPosX[l] > (MATRIX_WIDTH / 2)) {
      trackingObjectSpeedX[l] = -trackingObjectSpeedX[l];
    }
  }

  void LeapersMove_leaper(uint8_t l) {
#define GRAVITY 0.06
#define SETTLED_THRESHOLD 0.1
#define WALL_FRICTION 0.95
#define WIND 0.95  // wind resistance

    trackingObjectPosX[l] += trackingObjectSpeedX[l];
    trackingObjectPosY[l] += trackingObjectSpeedY[l];

    // bounce off the floor and ceiling?
    if (trackingObjectPosY[l] < 0 ||
        trackingObjectPosY[l] > MATRIX_HEIGHT - 1) {
      trackingObjectSpeedY[l] = (-trackingObjectSpeedY[l] * WALL_FRICTION);
      trackingObjectSpeedX[l] = (trackingObjectSpeedX[l] * WALL_FRICTION);
      trackingObjectPosY[l] += trackingObjectSpeedY[l];
      if (trackingObjectPosY[l] < 0)
        trackingObjectPosY[l] = 0;  // settled on the floor?
      if (trackingObjectPosY[l] <= SETTLED_THRESHOLD &&
          fabs(trackingObjectSpeedY[l]) <= SETTLED_THRESHOLD) {
        LeapersRestart_leaper(l);
      }
    }

    // bounce off the sides of the screen?
    if (trackingObjectPosX[l] <= 0 ||
        trackingObjectPosX[l] >= MATRIX_WIDTH - 1) {
      trackingObjectSpeedX[l] = (-trackingObjectSpeedX[l] * WALL_FRICTION);
      if (trackingObjectPosX[l] <= 0) {
        // trackingObjectPosX[l] = trackingObjectSpeedX[l]; // the bug?
        trackingObjectPosX[l] = -trackingObjectPosX[l];
      } else {
        // trackingObjectPosX[l] = WIDTH - 1 - trackingObjectSpeedX[l]; // the
        // bug?
        trackingObjectPosX[l] =
            MATRIX_WIDTH + MATRIX_WIDTH - 2 - trackingObjectPosX[l];
      }
    }

    trackingObjectSpeedY[l] -= GRAVITY;
    trackingObjectSpeedX[l] *= WIND;
    trackingObjectSpeedY[l] *= WIND;
  }

#if 1
  // функция получения цвета пикселя по его номеру
  uint32_t getPixColor(uint32_t thisSegm) const {
    uint32_t thisPixel = thisSegm;  // * SEGMENTS;
    if (thisPixel > NUM_LEDS - 1) return 0;
    return (((uint32_t)g()->leds[thisPixel].r << 16) |
            ((uint32_t)g()->leds[thisPixel].g << 8) |
            (uint32_t)g()
                ->leds[thisPixel]
                .b);  // а почему не просто return (leds[thisPixel])?
  }
#endif

  // функция получения цвета пикселя в матрице по его координатам
  CRGB getPixColorXY(uint8_t x, uint8_t y) {
    //  return getPixColor(g()->xy(x, y));
    return g()->leds[g()->xy(x, MATRIX_HEIGHT - 1 - y)];
    // return g()->leds[g()->xy(x, y)];
  }

  // функция отрисовки точки по координатам X Y
  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    // Mesmerizer flips the Y axis here.
    uint32_t thisPixel = g()->xy((uint8_t)x, MATRIX_HEIGHT - 1 - (uint8_t)y);
    g()->leds[thisPixel] = color;
  }

#undef WU_WEIGHT
  static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) {
    return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
  }

  //по мотивам
  // https://gist.github.com/sutaburosu/32a203c2efa2bb584f4b846a91066583
  void drawPixelXYF(float x, float y, CRGB color)  //, uint8_t darklevel = 0U)
  {
    //  if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть
    //  как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
    // extract the fractional parts and derive their inverses
    uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx,
            iy = 255 - yy;
    // calculate the intensities for each affected pixel
    // #define WU_WEIGHT(a,b) ((uint8_t) (((a)*(b)+(a)+(b))>>8))
    uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy),
                     WU_WEIGHT(xx, yy)};
    // multiply the intensities by the colour, and saturating-add them to the
    // pixels
    for (uint8_t i = 0; i < 4; i++) {
      int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
      CRGB clr = getPixColorXY(xn, yn);
      clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
      clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
      clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
      // if (darklevel) drawPixelXY(xn, yn, makeDarker(clr, darklevel));
      // else
      drawPixelXY(xn, yn, clr);
    }
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    g()->DimAll(181);

    for (uint8_t i = 0; i < MATRIX_WIDTH; i++) {
      for (uint8_t j = 0; j < MATRIX_HEIGHT; j++) {
        // leds[XY(i,HEIGHT-1U-j)] = ColorFromPalette(*curPalette,
        // qsub8(inoise8(i * deltaValue, (j+ff_y+random8(2)) * deltaHue, ff_z),
        // shiftHue[j]), 255U);
        drawPixelXYF(
            i, MATRIX_HEIGHT - 1U - j,
            ColorFromPalette(
                *curPalette,
                qsub8(inoise8(i * deltaValue,
                              (j + ff_y + random8(2)) * deltaHue, ff_z),
                      shiftHue[j]),
                255U));
      }
    }

    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      LeapersMove_leaper(i);
      // drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i],
      // CHSV(trackingObjectHue[i], 255U, 255U));
      if (trackingObjectPosY[i] >= MATRIX_HEIGHT / 4U)
        drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i],
                     ColorFromPalette(*curPalette, trackingObjectHue[i]));
    };

    // blurScreen(20);
    ff_y++;
    if (ff_y & 0x01) ff_z++;
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
