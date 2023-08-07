#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

#if ENABLE_AUDIO
class PatternSMSupernova : public BeatEffectBase,
                           public LEDStripEffect
#else
class PatternSMSupernova : public LEDStripEffect
#endif
{
 private:
  // matrix size constants are calculated only here and do not change in effects
  const uint8_t CENTER_X_MINOR =
      (MATRIX_WIDTH / 2) -
      ((MATRIX_WIDTH - 1) &
       0x01);  // the center of the matrix according to ICSU, shifted to the
               // smaller side, if the width is even
  const uint8_t CENTER_Y_MINOR =
      (MATRIX_HEIGHT / 2) -
      ((MATRIX_HEIGHT - 1) &
       0x01);  // center of the YGREK matrix, shifted down if the height is even
  const uint8_t CENTER_X_MAJOR =
      MATRIX_WIDTH / 2 +
      (MATRIX_WIDTH % 2);  // the center of the matrix according to IKSU,
                           // shifted to a larger side, if the width is even
  const uint8_t CENTER_Y_MAJOR =
      MATRIX_HEIGHT / 2 +
      (MATRIX_HEIGHT %
       2);  // center of the YGREK matrix, shifted up if the height is even
  uint8_t hue, hue2;  // постепенный сдвиг оттенка или какой-нибудь другой
                      // цикличный счётчик
  uint8_t step;  // какой-нибудь счётчик кадров или последовательностей операций
  uint8_t deltaValue;  // просто повторно используемая переменная
#define enlargedOBJECT_MAX_COUNT \
  (MATRIX_WIDTH * 2)  // максимальное количество сложных отслеживаемых объектов
                      // (меньше, чем MAX_COUNT)
  uint8_t enlargedObjectNUM;  // используемое в эффекте количество объектов
  long enlargedObjectTime[enlargedOBJECT_MAX_COUNT];
#define MAX_COUNT \
  (254U)  // максимальное количество отслеживаемых объектов (очень влияет на
          // расход памяти)
  float PosX[MAX_COUNT];
  float PosY[MAX_COUNT];
  uint8_t Hue[MAX_COUNT];
  uint8_t State[MAX_COUNT];
  float SpeedX[MAX_COUNT];
  float SpeedY[MAX_COUNT];
  bool IsShift[MAX_COUNT];  // BUGBUG: Could be a Std::Bitfield<>

  // неточный, зато более быстрый квадратный корень
  float sqrt3(const float x) {
    union {
      int i;
      float x;
    } u;

    u.x = x;
    u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
    return u.x;
  }

  //-------- по мотивам Эффектов Particle System -------------------------
  // https://github.com/fuse314/arduino-particle-sys
  // https://github.com/giladaya/arduino-particle-sys
  // https://www.youtube.com/watch?v=S6novCRlHV8&t=51s
  //#include <ParticleSys.h>
  //при попытке вытащить из этой библиотеки только минимально необходимое
  //выяснилось, что там очередной (третий) вариант реализации субпиксельной
  //графики. ну его нафиг. лучше будет повторить визуал имеющимися в прошивке
  //средствами.

  void particlesUpdate2(uint8_t i) {
    // apply velocity
    PosX[i] += SpeedX[i];
    PosY[i] += SpeedY[i];
    if (State[i] == 0 || PosX[i] <= -1 || PosX[i] >= MATRIX_WIDTH ||
        PosY[i] <= -1 || PosY[i] >= MATRIX_HEIGHT)
      IsShift[i] = false;
  }

  // ============= ЭФФЕКТ ИСТОЧНИК ===============
  // (c) SottNick
  // выглядит как
  // https://github.com/fuse314/arduino-particle-sys/blob/master/examples/StarfieldFastLED/StarfieldFastLED.ino
  void starfield2Emit(uint8_t i) {
    if (hue++ & 0x01) hue2 += 1;  // counter++;
    // source->update(g); хз зачем это было в оригинале - там только смерть
    // source.isAlive высчитывается, вроде
    PosX[i] = MATRIX_WIDTH * 0.5;  // CENTER_X_MINOR;// * RENDERER_RESOLUTION;
                                   // //  particle->x = source->x;
    PosY[i] = MATRIX_HEIGHT * 0.5;  // CENTER_Y_MINOR;// * RENDERER_RESOLUTION;
                                    // //  // particle->y = source->y;
    SpeedX[i] = (((float)random8() - 127.) /
                 512.);  // random(_hVar)-_constVel; // particle->vx
    SpeedY[i] = sqrt(
        0.0626 -
        SpeedX[i] *
            SpeedX[i]);  // sqrt(pow(_constVel,2)-pow(SpeedX[i],2)); //
                         // particle->vy зависит от particle->vx - не ошибка
    if (random8(2U)) {
      SpeedY[i] = -SpeedY[i];
    }
    State[i] = random8(1, 250);  // random8(minLife, maxLife);// particle->ttl
    Hue[i] = hue2;               // (counter/2)%255; // particle->hue
    IsShift[i] = true;           // particle->isAlive
  }

  [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const {
    return g()->leds[g()->xy(x, MATRIX_HEIGHT - 1 - y)];
    // return g()->leds[g()->xy(x, y)];
  }

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
      drawPixelXY(xn, yn, clr);
    }
  }

 public:
  PatternSMSupernova()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMSUPERNOVA, "Supernova") {
  }

  PatternSMSupernova(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();
    enlargedObjectNUM = 200;

    if (enlargedObjectNUM > MAX_COUNT) enlargedObjectNUM = 200U;  // MAX_COUNT;
    deltaValue = enlargedObjectNUM / (sqrt3(CENTER_X_MAJOR * CENTER_X_MAJOR +
                                            CENTER_Y_MAJOR * CENTER_Y_MAJOR) *
                                      4U) +
                 1U;  // 4 - это потому что за 1 цикл частица пролетает ровно
                      // четверть расстояния между 2мя соседними пикселями
    for (int i = 0; i < enlargedObjectNUM; i++)
      IsShift[i] = false;  // particle->isAlive
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    step = -1;  // deltaValue; //счётчик количества частиц в очереди на
                // зарождение в этом цикле
    g()->DimAll(200);
    // go over particles and update matrix cells on the way
    for (int i = 0; i < enlargedObjectNUM; i++) {
      if (!IsShift[i] && step) {
        starfield2Emit(i);
        step -= 1;
      }
      if (IsShift[i]) {  // particle->isAlive
        // particles[i].update(this->g);
        particlesUpdate2(i);

        // generate RGB values for particle
        //      CRGB baseRGB = CHSV(Hue[i], 255,255); // particles[i].hue
        CRGB baseRGB = ColorFromPalette(HeatColors_p, Hue[i], 255, LINEARBLEND);
        // baseRGB.fadeToBlackBy(255-State[i]);
        baseRGB.nscale8(State[i]);  //эквивалент
        drawPixelXYF(PosX[i], PosY[i], baseRGB);
      }
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
