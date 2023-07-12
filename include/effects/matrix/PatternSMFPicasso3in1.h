#pragma once
#undef trackingOBJECT_MAX_COUNT

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Inspired by https://editor.soulmatelights.com/gallery/1177-picasso-3in1

#if ENABLE_AUDIO
class PatternSMPicasso3in1 : public BeatEffectBase,
                             public LEDStripEffect
#else
class PatternSMPicasso3in1 : public LEDStripEffect
#endif
{
 private:
  // Suggested values for Mesmerizer w/ 1/2 HUB75 panel: 10, 36, 70
  uint8_t Scale =
      2;  // 1-100 is image type and count. THis should be a Setting 0-33 = P1,
          // 34-68 = P2, 68-99 = Picasso3 P1 - Scale is number of independent
          // lines drawn w/ trailers. Safe=1-8 20 or so = Pink Floyd 'Pulse'
          // laser show. P2 - 34 & up. Scale -33 == number of vertices on a
          // connected polyline "wire" rotating in 3d. 38 up? SLOW! P3 - 68 &
          // up. Scale -68 -2 == number of circles  *68=2, 69=3, 70=4, etc. 80
          // up? SLOW
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

  // функция отрисовки точки по координатам X Y
  void drawPixelXYF(float x, float y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    if (x < 1 || x > (MATRIX_WIDTH - 2) || y < 1 || y > (MATRIX_HEIGHT - 2))
      return;
    // uint32_t thisPixel = g()->xy((uint8_t)x, (uint8_t)y);
    // NightDriver's coordinate system is dfferent. Invert height and this all
    // works!
    uint32_t thisPixel = g()->xy((uint8_t)x, MATRIX_HEIGHT - (uint8_t)y);
    g()->leds[thisPixel] = color;
  }
  // функция отрисовки точки по координатам X Y
  void drawPixelXY(int x, int y, CRGB color) { drawPixelXYF(x, y, color); }
  // ------------------------------ Дополнительные функции рисования
  // ----------------------
  void DrawLine(int x1, int y1, int x2, int y2, CRGB color) {
    int deltaX = abs(x2 - x1);
    int deltaY = abs(y2 - y1);
    int signX = x1 < x2 ? 1 : -1;
    int signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;

    drawPixelXY(x2, y2, color);
    while (x1 != x2 || y1 != y2) {
      drawPixelXY(x1, y1, color);
      int error2 = error * 2;
      if (error2 > -deltaY) {
        error -= deltaY;
        x1 += signX;
      }
      if (error2 < deltaX) {
        error += deltaX;
        y1 += signY;
      }
    }
  }

  void DrawLineF(float x1, float y1, float x2, float y2, CRGB color) {
    float deltaX = fabs(x2 - x1);
    float deltaY = fabs(y2 - y1);
    float error = deltaX - deltaY;

    float signX = x1 < x2 ? 0.5 : -0.5;
    float signY = y1 < y2 ? 0.5 : -0.5;

    while (x1 != x2 ||
           y1 !=
               y2) {  // (true) - а я то думаю - "почему функция часто вызывает
                      // вылет по вачдогу?" А оно вон оно чё, Михалычь!
      if ((signX > 0 && x1 > x2 + signX) || (signX < 0 && x1 < x2 + signX))
        break;
      if ((signY > 0 && y1 > y2 + signY) || (signY < 0 && y1 < y2 + signY))
        break;
      drawPixelXYF(
          x1, y1,
          color);  // интересно, почему тут было обычное drawPixelXY() ???
      float error2 = error;
      if (error2 > -deltaY) {
        error -= deltaY;
        x1 += signX;
      }
      if (error2 < deltaX) {
        error += deltaX;
        y1 += signY;
      }
    }
  }

  // We use our own drawCircle() and drawPixel() because we KNOW we're going to
  // draw near edges and the system versions scribble on memory when we do. Ours
  // clamp.
  void drawCircle(int x0, int y0, int radius, const CRGB& color) {
    int a = radius, b = 0;
    int radiusError = 1 - a;

    if (radius == 0) {
      drawPixelXY(x0, y0, color);
      return;
    }

    while (a >= b) {
      drawPixelXY(a + x0, b + y0, color);
      drawPixelXY(b + x0, a + y0, color);
      drawPixelXY(-a + x0, b + y0, color);
      drawPixelXY(-b + x0, a + y0, color);
      drawPixelXY(-a + x0, -b + y0, color);
      drawPixelXY(-b + x0, -a + y0, color);
      drawPixelXY(a + x0, -b + y0, color);
      drawPixelXY(b + x0, -a + y0, color);
      b++;
      if (radiusError < 0)
        radiusError += 2 * b + 1;
      else {
        a--;
        radiusError += 2 * (b - a + 1);
      }
    }
  }

  void PicassoGenerate(bool reset) {
    static int loadingFlag = true;
    if (loadingFlag) {
      loadingFlag = false;
      if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT)
        enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
      if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

      double minSpeed = 0.2, maxSpeed = 0.8;

      for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
        trackingObjectPosX[i] = random8(MATRIX_WIDTH);
        trackingObjectPosY[i] = random8(MATRIX_HEIGHT);

        // curr->color = CHSV(random(1U, 255U), 255U, 255U);
        trackingObjectHue[i] = random8();

        trackingObjectSpeedY[i] =
            +((-maxSpeed / 3) + (maxSpeed * (float)random8(1, 100) / 100));
        trackingObjectSpeedY[i] +=
            trackingObjectSpeedY[i] > 0 ? minSpeed : -minSpeed;

        trackingObjectShift[i] =
            +((-maxSpeed / 2) + (maxSpeed * (float)random8(1, 100) / 100));
        trackingObjectShift[i] +=
            trackingObjectShift[i] > 0 ? minSpeed : -minSpeed;

        trackingObjectState[i] = trackingObjectHue[i];
      }
    }
    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      if (reset) {
        trackingObjectState[i] = random8();
        trackingObjectSpeedX[i] =
            (trackingObjectState[i] - trackingObjectHue[i]) / 25;
      }
      if (trackingObjectState[i] != trackingObjectHue[i] &&
          trackingObjectSpeedX[i]) {
        trackingObjectHue[i] += trackingObjectSpeedX[i];
      }
    }
  }

  void PicassoPosition() {
    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      if (trackingObjectPosX[i] + trackingObjectSpeedY[i] > MATRIX_WIDTH ||
          trackingObjectPosX[i] + trackingObjectSpeedY[i] < 0) {
        trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
      }

      if (trackingObjectPosY[i] + trackingObjectShift[i] > MATRIX_HEIGHT ||
          trackingObjectPosY[i] + trackingObjectShift[i] < 0) {
        trackingObjectShift[i] = -trackingObjectShift[i];
      }

      trackingObjectPosX[i] += trackingObjectSpeedY[i];
      trackingObjectPosY[i] += trackingObjectShift[i];
    };
  }

  void PicassoRoutine1() {
    PicassoGenerate(false);
    PicassoPosition();

    for (uint8_t i = 0; i < enlargedObjectNUM - 2U; i += 2) {
      DrawLine(trackingObjectPosX[i], trackingObjectPosY[i],
               trackingObjectPosX[i + 1U], trackingObjectPosY[i + 1U],
               CHSV(trackingObjectHue[i], 255U, 255U));
      // DrawLine(trackingObjectPosX[i], trackingObjectPosY[i],
      // trackingObjectPosX[i+1U], trackingObjectPosY[i+1U],
      // ColorFromPalette(*curPalette, trackingObjectHue[i]));
    }

    g()->BlurFrame(80);
  }

  void PicassoRoutine2() {
    PicassoGenerate(false);
    PicassoPosition();

    g()->DimAll(180);

    for (uint8_t i = 0; i < enlargedObjectNUM - 1U; i++)
      DrawLineF(trackingObjectPosX[i], trackingObjectPosY[i],
                trackingObjectPosX[i + 1U], trackingObjectPosY[i + 1U],
                CHSV(trackingObjectHue[i], 255U, 255U));

    EVERY_N_MILLIS(20000) { PicassoGenerate(true); }
    g()->BlurFrame(80);
  }

  void PicassoRoutine3() {
    PicassoGenerate(false);
    PicassoPosition();
    g()->DimAll(180);

    for (uint8_t i = 0; i < enlargedObjectNUM - 2U; i += 2)
      drawCircle(fabs(trackingObjectPosX[i] - trackingObjectPosX[i + 1U]),
                 fabs(trackingObjectPosY[i] - trackingObjectPosX[i + 1U]),
                 fabs(trackingObjectPosX[i] - trackingObjectPosY[i]),
                 CHSV(trackingObjectHue[i], 255U, 255U));

    EVERY_N_MILLIS(20000) { PicassoGenerate(true); }
    g()->BlurFrame(80);
  }

 public:
  PatternSMPicasso3in1()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMPICASSO3IN1, "Picasso 3in1") {
  }

  PatternSMPicasso3in1(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  // This has atomicity issues. It looks at a global (boooo) that may change the
  // contents of the underlying enlarge/trackingFOO arrays while they're being
  // traversed. It's just a bad diea to modify Scale while these are running,
  // but there's too much cool code in this effect to just let it go to waste.

  void RecalibrateDrawnObjects() {
    if (Scale < 34U)  // если масштаб до 34
      enlargedObjectNUM =
          (Scale - 1U) / 32.0 * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
    else if (Scale >= 68U)  // если масштаб больше 67
      enlargedObjectNUM =
          (Scale - 68U) / 32.0 * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
    else  // для масштабов посередине
      enlargedObjectNUM =
          (Scale - 34U) / 33.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
  }

  virtual void Start() override {
    g()->Clear();
    RecalibrateDrawnObjects();
  }

  virtual void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    // Mesmerizer/NightDriver demo: just pick some preset and skip through them.
    // Good way to demo off this module, but it's a bit much visually!
    // robert
    // An entry of 6 does so much work it trips the watchdog!
    // An entry of 45 does so much work it trips the watchdog!
    static int demo_values[] = {3, 35, 68};
    static int demo_idx = 0;

    // BUGBUG(robertl): Just eyeballing this, I think the clock is running about
    // double time...
    EVERY_N_MILLIS(2000) {
      if (demo_idx++ == sizeof(demo_values) / sizeof(demo_values[0]))
        demo_idx = 0;
      Scale = demo_values[demo_idx];
      // RecalibrateDrawnObjects();
      debugI("Index %d Scale %d", demo_idx, Scale);
      // delay(10);
    }
    // end robert

    // Don't just let the renderer freewheel.
    // Unfortunately, this is a terrible way to implement usage governors.
    EVERY_N_MILLIS(16) {
      if (Scale < 34U)  // если масштаб до 34
        PicassoRoutine1();
      else if (Scale > 67U)  // если масштаб больше 67
        PicassoRoutine3();
      else  // для масштабов посередине
        PicassoRoutine2();
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
