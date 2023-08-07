#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// https://editor.soulmatelights.com/gallery/1118-snakes

#if ENABLE_AUDIO
class PatternSMStrobeDiffusion : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMStrobeDiffusion : public LEDStripEffect
#endif
{
private:
  uint8_t Speed = 150;       // 1-255
  uint8_t Scale = 20;        // 1-100
public:
  PatternSMStrobeDiffusion() :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMSTROBE_DIFFUSION, "Strobe Diffusion")
    {
    }

  PatternSMStrobeDiffusion(const JsonObjectConst& jsonObject) :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  virtual void Start() override
  {
    g()->Clear();
       speedfactor = (float)modes[currentMode].Speed / 555.0f + 0.001f;

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    for (uint8_t i = 0; i < enlargedObjectNUM; i++){
      enlargedObjectTime[i] = 0;
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = (255. + random8()) / 255.;
      trackingObjectSpeedY[i] = 0;
      //trackingObjectShift[i] = 0;
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(4);//     B00           направление головы змейки
                                          // B10     B11
                                          //     B01
  }

  virtual void Draw() override
  {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

  g()->clear();

  int8_t dx, dy;
  for (uint8_t i = 0; i < enlargedObjectNUM; i++){
   trackingObjectSpeedY[i] += trackingObjectSpeedX[i] * speedfactor;
   if (trackingObjectSpeedY[i] >= 1)
   {
    trackingObjectSpeedY[i] = trackingObjectSpeedY[i] - (int)trackingObjectSpeedY[i];
    if (random8(9U) == 0U) // вероятность поворота
      if (random8(2U)){ // <- поворот налево
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2) | B01; // младший бит = поворот
        switch (trackingObjectState[i]) {
          case B10:
            trackingObjectState[i] = B01;
            if (trackingObjectPosY[i] == 0U)
              trackingObjectPosY[i] = HEIGHT - 1U;
            else
              trackingObjectPosY[i]--;
            break;
          case B11:
            trackingObjectState[i] = B00;
            if (trackingObjectPosY[i] >= HEIGHT - 1U)
              trackingObjectPosY[i] = 0U;
            else
              trackingObjectPosY[i]++;
            break;
          case B00:
            trackingObjectState[i] = B10;
            if (trackingObjectPosX[i] == 0U)
              trackingObjectPosX[i] = WIDTH - 1U;
            else
              trackingObjectPosX[i]--;
            break;
          case B01:
            trackingObjectState[i] = B11;
            if (trackingObjectPosX[i] >= WIDTH - 1U)
              trackingObjectPosX[i] = 0U;
            else
              trackingObjectPosX[i]++;
            break;
        }
      }
      else{ // -> поворот направо
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2) | B11; // младший бит = поворот, старший = направо
        switch (trackingObjectState[i]) {
          case B11:
            trackingObjectState[i] = B01;
            if (trackingObjectPosY[i] == 0U)
              trackingObjectPosY[i] = HEIGHT - 1U;
            else
              trackingObjectPosY[i]--;
            break;
          case B10:
            trackingObjectState[i] = B00;
            if (trackingObjectPosY[i] >= HEIGHT - 1U)
              trackingObjectPosY[i] = 0U;
            else
              trackingObjectPosY[i]++;
            break;
          case B01:
            trackingObjectState[i] = B10;
            if (trackingObjectPosX[i] == 0U)
              trackingObjectPosX[i] = WIDTH - 1U;
            else
              trackingObjectPosX[i]--;
            break;
          case B00:
            trackingObjectState[i] = B11;
            if (trackingObjectPosX[i] >= WIDTH - 1U)
              trackingObjectPosX[i] = 0U;
            else
              trackingObjectPosX[i]++;
            break;
        }
      }
    else { // двигаем без поворота
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2);
        switch (trackingObjectState[i]) {
          case B01:
            if (trackingObjectPosY[i] == 0U)
              trackingObjectPosY[i] = HEIGHT - 1U;
            else
              trackingObjectPosY[i]--;
            break;
          case B00:
            if (trackingObjectPosY[i] >= HEIGHT - 1U)
              trackingObjectPosY[i] = 0U;
            else
              trackingObjectPosY[i]++;
            break;
          case B10:
            if (trackingObjectPosX[i] == 0U)
              trackingObjectPosX[i] = WIDTH - 1U;
            else
              trackingObjectPosX[i]--;
            break;
          case B11:
            if (trackingObjectPosX[i] >= WIDTH - 1U)
              trackingObjectPosX[i] = 0U;
            else
              trackingObjectPosX[i]++;
            break;
        }

    }
   }

    switch (trackingObjectState[i]) {
     case B01:
       dy = 1;
       dx = 0;
       break;
     case B00:
       dy = -1;
       dx = 0;
       break;
     case B10:
       dy = 0;
       dx = 1;
       break;
     case B11:
       dy = 0;
       dx = -1;
       break;
    }
    long temp = enlargedObjectTime[i];
    uint8_t x = trackingObjectPosX[i];
    uint8_t y = trackingObjectPosY[i];
    //CHSV color = CHSV(trackingObjectHue[i], 255U, 255U);
    //drawPixelXY(x, y, color);
    //drawPixelXYF(x, y, CHSV(trackingObjectHue[i], 255U, trackingObjectSpeedY[i] * 255)); // тут рисуется голова // слишком сложно для простого сложения цветов
    leds[XY(x,y)] += CHSV(trackingObjectHue[i], 255U, trackingObjectSpeedY[i] * 255); // тут рисуется голова

    for (uint8_t m = 0; m < SNAKES_LENGTH; m++){ // 16 бит распаковываем, 14 ещё остаётся без дела в запасе, 2 на хвостик
      x = (WIDTH + x + dx) % WIDTH;
      y = (HEIGHT + y + dy) % HEIGHT;
      //drawPixelXYF(x, y, CHSV(trackingObjectHue[i] + m*4U, 255U, 255U)); // тут рисуется тело // слишком сложно для простого сложения цветов
      //leds[XY(x,y)] += CHSV(trackingObjectHue[i] + m*4U, 255U, 255U); // тут рисуется тело
      leds[XY(x,y)] += CHSV(trackingObjectHue[i] + (m + trackingObjectSpeedY[i])*4U, 255U, 255U); // тут рисуется тело

      if (temp & B01){ // младший бит = поворот, старший = направо
        temp = temp >> 1;
        if (temp & B01){ // старший бит = направо
          if (dx == 0){
            dx = 0 - dy;
            dy = 0;
          }
          else{
            dy = dx;
            dx = 0;
          }
        }
        else{ // иначе налево
          if (dx == 0){
            dx = dy;
            dy = 0;
          }
          else{
            dy = 0 - dx;
            dx = 0;
          }
        }
        temp = temp >> 1;
      }
      else { // если без поворота
        temp = temp >> 2;
      }
    }
    x = (WIDTH + x + dx) % WIDTH;
    y = (HEIGHT + y + dy) % HEIGHT;
    //drawPixelXYF(x, y, CHSV(trackingObjectHue[i] + SNAKES_LENGTH*4U, 255U, (1 - trackingObjectSpeedY[i]) * 255)); // хвостик // слишком сложно для простого сложения цветов
    //leds[XY(x,y)] += CHSV(trackingObjectHue[i] + SNAKES_LENGTH*4U, 255U, (1 - trackingObjectSpeedY[i]) * 255); // хвостик
    leds[XY(x,y)] += CHSV(trackingObjectHue[i] + (SNAKES_LENGTH + trackingObjectSpeedY[i])*4U, 255U, (1 - trackingObjectSpeedY[i]) * 255); // хвостик
  }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override
  {

  }
#endif
};
