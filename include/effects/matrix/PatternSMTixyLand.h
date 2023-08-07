#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/916-tixyland
// 37 (!) mathematically easy effects in one. Pick your favorites and extract
// them. It's the kitchen sink. Some are quite cool.

#if ENABLE_AUDIO
class PatternSMTixyLand : public BeatEffectBase,
                          public LEDStripEffect
#else
class PatternSMTixyLand : public LEDStripEffect
#endif
{
 private:
  static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;
  // Sample code taken from:
  // https://editor.soulmatelights.com/gallery/671-tixyland

  // https://tixy.land/?code=sin%28sin%28y%29%2Bx%2Bt%29*cos%28t%2By%29
  // https://tixy.land/?code=tan%28sqrt%28sin%28x%29*16*sin%28x%29*16%2Bcos%28y%29*16*cos%28y%29*16%29%2Bt%29
  // https://tixy.land/?code=sin%28x%2Bsin%28y%2F2%2Bt*5%29%29%2Bcos%28y%29
  // https://tixy.land/?code=sin%28sin%28x%2Bt%29%2By%2Bt*10%29-y%2F10
  // https://tixy.land/?code=sin%28sin%28y%2Bt%29%2Bx%2Bt*10%29-y%2F10

  byte effect = 0;
  uint8_t gHue = 0;

  float code(double t, double i, double x, double y) {
    // put  tixy.land formulas after return
    // use fmod() against C++ modulo %
    // float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    // // use against random()

    // some patterns from twitter https://twitter.com/search?q=tixy.land&s=09 &
    // reddit. uncomment it

    //____________________________
    // float a = tan(-y/max(x-8,y-8,7-x,7-y)-t*4);
    // //http://maettig.com/code/canvas/tixy.land-hd.html?code=tan%28-y%2Fmax%28x-8%2Cy-8%2C7-x%2C7-y%29-t*4%29
    // float a = max(x-8,y-8);                       //uncomment whole block, in
    // arduino max() work only with 2 numbers instead javascript ( a =
    // max(a,7-x); a = max (a,7-y); a = tan(-y/a-t*4); return a;
    //_____________________________________________
    switch (effect) {
      case 0:
        return x / y * (hypot(x + cos(t) - 8, y - 7) - 6);
        break;  // https://twitter.com/aemkei/status/1340044770257870851?s=20 //
                // shady sphere// by @blu3_enjoy
      case 1:
        return .1 / (sin(y / 4 - t * 6) - sin(x * 2 - t));
        break;  ////https://twitter.com/ntsutae/status/1336729037549436931?s=20
      case 2:
        return sin(hypot(x -= 8, y -= 8) + t + atan2(y, x));
        break;  // https://twitter.com/aemkei/status/1331340057727406081?s=20
      case 3:
        return abs(cos(atan2(y + 2, x - 8) * 6 + t * 2));
        break;  // https://twitter.com/mgmalheiros/status/1328456054049476611?s=20
      case 4:
        return tan(cos(x) - t) * tan(sin(y) - t) / 16;
        break;  // https://twitter.com/SonezakiRinji/status/1328149756133052416?s=20
      case 5:
        return sin(y * sin(t / 2) * 2 + t + i * 6);
        break;
      case 6:
        return abs(7 - x) < 9 ? cos(t + (x + y) * PI / 8) : 0;
        break;  // https://twitter.com/HiraginoYuki/status/1327166558955663362
      case 7:
        return !((int)(x + t * 50 / (fmod(y * y, 5.9) + 1)) & 15) /
               (fmod(y * y, 5.9) + 1);
        break;
      case 8:
        return sin(atan((y - 7.5) / (x - 7.5)) + t * 6);
        break;
      case 9:
        return sin(atan((y) / (x)) + t);
        break;
      case 10:
        return 1 - fmod(((t + x + sin(t + x) / 2) - y / 30), 1.0);
        break;
      case 11:
        return (y - 8) / 3 - tan(x / 6 + 1.87) * sin(t * 2);
        break;
      case 12:
        return (y - 8) / 3 - (sin(x / 4 + t * 4));
        break;
      case 13:
        return fmod(i, 4) - fmod(y, 4) + sin(t);
        break;
      case 14:
        return sin(x) - sin(x / 2 - 2 * t) - y / 1.2 + 6;
        break;
      case 15:
        return -.4 / (hypot(x - fmod(t, 10), y - fmod(t, 8)) - fmod(t, 2) * 9);
        break;
      case 16:
        return sin(x / 3 * sin(t / 3) * 2) + cos(y / 4 * sin(t / 2) * 2);
        break;
      case 17:
        return cos(sin((x * t * .1)) * PI) +
               cos(sin(y * t * .1 + (sqrt(abs(cos(x * t * .1))))) * PI);
        break;
      case 18:
        return x / t / sqrt(55 - (x -= 8) * x - (y -= 8) * y);
        break;  // https://twitter.com/XorDev/status/1327059496951291905
      case 19:
        return sin(x * x * 3 * i / 1e4 - y / 2 + t * 9);
        break;
      case 20:
        return std::min(7 - y + sin(x + sin(y + t * 8)) * 6, 0.0);
        break;  // fire https://twitter.com/davemakes/status/1324226447351803905
      case 21:
        return x * y / 64 * sin(t + x * 6 - y * 6);
        break;  // https://twitter.com/maettig/status/1326162655061696513
      case 22:
        return 6 - hypot(x - 7.5, y - 7.5) - sin(i / 3 - t);
        break;  // https://twitter.com/maettig/status/1326162655061696513
      case 23:
        return 1 - abs((x - 6) * cos(t) + (y - 6) * sin(t));
        break;  // https://twitter.com/maettig/status/1326163017533419529
      case 24:
        return atan((x - 7.5) * (y - 7.5)) - 2.5 * sin(t);
        break;  // https://twitter.com/maettig/status/1326163136559403015
      case 25:
        return sin(3 * atan2(y - 7.5, x - 7.5) + t);
        break;  // https://twitter.com/aemkei/status/1326637631409676291
      case 26:
        return sin(3 * atan2(y - 7.5 + sin(t) * 5, x - 7.5 + sin(t / 2) * 5) +
                   t * 5);
        break;  // i add move for
                // //https://twitter.com/aemkei/status/1326637631409676291
      case 27:
        return sin(PI * 2 * atan((y - 8) / (x - 8)) + 5 * t);
        break;
      case 28:
        return sin((t / 16) * i + x / y);
        break;
      case 29:
        return sin(x / y - y / x + t);
        break;
      case 30:
        return sin(t - sqrt(x * x + y * y));
        break;
      case 31:
        return sin(x + t) + sin(y + t) + sin(x + y + t) / 3;
        break;
      case 32:
        return 1 - hypot(sin(t) * 9 - x, cos(t) * 9 - y) / 9;
        break;
      case 33:
        return 1 - fmod((x * x - y + t * (fmod(1 + x * x, 5.0)) * 3.0), 16.0) /
                       16.0;
        break;
      case 34:
        return sin(sin(y) + x + t) * cos(t + y);
        break;
      case 35:
        return sin(x + sin(y / 2 + t * 5)) + cos(y);
        break;
      case 36:
        return x > 7 - (sin(t * 5 + y * 3) * 6) ? 1 : .2;
        break;  // Ldir's DNA )))
                // https://twitter.com/ldir_ko/status/1326099121170771968
                // https://tixy.land/?code=x%3E7-%28sin%28t*5%2By*3%29*6%29%3F1%3A.2
      default:
        return 0;
    }
    //_____________________
    // float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    // // my fire )) https://twitter.com/ldir_ko/status/1325720180262113281
    // https://tixy.land/?code=%7Ei%2F200*%28y%2F40%2Frandom%28%29%29 return
    // -i/200*(y/40/r);   // my fire ))
    //____________________________________
  }

  void processFrame(double t, double x, double y) {
    EVERY_N_MILLISECONDS(30) { gHue++; }
    double i = (y * 16) + x;
    double frame = constrain(code(t, i, x, y), -1, 1) * 255;
    if (frame >= 0) {
      g()->leds[g()->xy(x, (LED_ROWS - 1 - y))] = CHSV(gHue, 255, frame);
    }  // change to XY(x, y) for non rotate display
    else {
      g()->leds[g()->xy(x, (LED_ROWS - 1 - y))] = CHSV(gHue + 55, 255, -frame);
    }  // change to XY(x, y) for non rotate display
  }

 public:
  PatternSMTixyLand()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMTIXY_LAND, "TixyLand") {
  }

  PatternSMTixyLand(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Label(int n) {
    String result = str_sprintf("Tixy %d", n);
    const char* pszText = result.c_str();

    LEDMatrixGFX::backgroundLayer.setFont(gohufont11b);
    int x = 10;
    int y = 10;
    LEDMatrixGFX::backgroundLayer.drawString(x - 1, y, rgb24(0, 0, 0), pszText);
    LEDMatrixGFX::backgroundLayer.drawString(x + 1, y, rgb24(0, 0, 0), pszText);
    LEDMatrixGFX::backgroundLayer.drawString(x, y - 1, rgb24(0, 0, 0), pszText);
    LEDMatrixGFX::backgroundLayer.drawString(x, y + 1, rgb24(0, 0, 0), pszText);
    LEDMatrixGFX::backgroundLayer.drawString(x, y, rgb24(255, 255, 255),
                                             pszText);
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    double t =
        millis() /
        1000.0;  // some formulas is hardcoded and fps get down. this speedup it
    for (byte x = 0; x < LED_COLS; x++) {
      for (byte y = 0; y < LED_ROWS; y++) {
        processFrame(t, x, y);
      }
    }
    EVERY_N_SECONDS(10) {  // 10 is too fast...
      effect++;
      if (effect > 36) effect = 0;
      Label(
          effect);  // This will only last one frame. This is already too slow.
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
