#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/916-tixyland
// 37 (!) mathematically easy effects in one. Pick your favorites and extract
// them. It's the kitchen sink. Some are quite cool.

class PatternSMTixyLand : public LEDStripEffect
{
  private:
    // Sample code taken from:
    // https://editor.soulmatelights.com/gallery/671-tixyland

    // https://tixy.land/?code=sin%28sin%28y%29%2Bx%2Bt%29*cos%28t%2By%29
    // https://tixy.land/?code=tan%28sqrt%28sin%28x%29*16*sin%28x%29*16%2Bcos%28y%29*16*cos%28y%29*16%29%2Bt%29
    // https://tixy.land/?code=sin%28x%2Bsin%28y%2F2%2Bt*5%29%29%2Bcos%28y%29
    // https://tixy.land/?code=sin%28sin%28x%2Bt%29%2By%2Bt*10%29-y%2F10
    // https://tixy.land/?code=sin%28sin%28y%2Bt%29%2Bx%2Bt*10%29-y%2F10

    byte effect = 0;
    uint8_t gHue = 0;

    float code(float t, float i, float x, float y)
    {
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
        switch (effect)
        {
        case 0:
            return x / y * (hypot(x + cos(t) - 8, y - 7) - 6);
            break; // https://twitter.com/aemkei/status/1340044770257870851?s=20 //
                   // shady sphere// by @blu3_enjoy
        case 1:
            return .1 / (sin8(y / 4 - t * 6) - sin(x * 2 - t));
            break; ////https://twitter.com/ntsutae/status/1336729037549436931?s=20
        case 2:
            return sinf(hypotf(x -= 8, y -= 8) + t + atan2f(y, x));
            break; // https://twitter.com/aemkei/status/1331340057727406081?s=20
        case 3:
            return fabs(cosf(atan2f(y + 2, x - 8) * 6 + t * 2));
            break; // https://twitter.com/mgmalheiros/status/1328456054049476611?s=20
        case 4:
            return tanf(cosf(x) - t) * tanf(sinf(y) - t) / 16;
            break; // https://twitter.com/SonezakiRinji/status/1328149756133052416?s=20
        case 5:
            return sinf(y * sinf(t / 2) * 2 + t + i * 6);
            break;
        case 6:
            return fabs(7 - x) < 9 ? cosf(t + (x + y) * (float)PI / 8) : 0;
            break; // https://twitter.com/HiraginoYuki/status/1327166558955663362
        case 7:
            return !((int)(x + t * 50 / (fmod(y * y, 5.9f) + 1)) & 15) / (fmod(y * y, 5.9f) + 1);
            break;
        case 8:
            return sinf(atanf((y - 7.5f) / (x - 7.5f)) + t * 6);
            break;
        case 9:
            return sinf(atanf((y) / (x)) + t);
            break;
        case 10:
            return 1 - fmod(((t + x + sinf(t + x) / 2) - y / 30), 1.0f);
            break;
        case 11:
            return (y - 8) / 3 - tanf(x / 6 + 1.87f) * sinf(t * 2);
            break;
        case 12:
            return (y - 8) / 3 - (sinf(x / 4 + t * 4));
            break;
        case 13:
            return fmod(i, 4) - fmod(y, 4) + sinf(t);
            break;
        case 14:
            return sinf(x) - sinf(x / 2 - 2 * t) - y / 1.2f + 6;
            break;
        case 15:
            return -.4f / (hypotf(x - fmod(t, 10), y - fmod(t, 8)) - fmod(t, 2) * 9);
            break;
        case 16:
            return sinf(x / 3 * sinf(t / 3) * 2) + cos(y / 4 * sin(t / 2) * 2);
            break;
        case 17:
            return cosf(sinf((x * t * .1f)) * (float)PI) +
                   cosf(sinf(y * t * .1f + (sqrtf(fabs(cosf(x * t * .1))))) * (float)PI);
            break;
        case 18:
            return x / t / sqrtf(55 - (x -= 8) * x - (y -= 8) * y);
            break; // https://twitter.com/XorDev/status/1327059496951291905
        case 19:
            return sinf(x * x * 3 * i / 1e4 - y / 2 + t * 9);
            break;
        case 20:
            return std::min(7 - y + sinf(x + sinf(y + t * 8)) * 6, 0.0f);
            break; // fire https://twitter.com/davemakes/status/1324226447351803905
        case 21:
            return x * y / 64 * sinf(t + x * 6 - y * 6);
            break; // https://twitter.com/maettig/status/1326162655061696513
        case 22:
            return 6 - hypotf(x - 7.5, y - 7.5) - sinf(i / 3 - t);
            break; // https://twitter.com/maettig/status/1326162655061696513
        case 23:
            return 1 - fabs((x - 6) * cosf(t) + (y - 6) * sinf(t));
            break; // https://twitter.com/maettig/status/1326163017533419529
        case 24:
            return atanf((x - 7.5f) * (y - 7.5f)) - 2.5f * sinf(t);
            break; // https://twitter.com/maettig/status/1326163136559403015
        case 25:
            return sinf(3 * atan2f(y - 7.5f, x - 7.5f) + t);
            break; // https://twitter.com/aemkei/status/1326637631409676291
        case 26:
            return sinf(3 * atan2f(y - 7.5f + sinf(t) * 5, x - 7.5f + sinf(t / 2) * 5) + t * 5);
            break; // i add move for
                   // //https://twitter.com/aemkei/status/1326637631409676291
        case 27:
            return sinf((float)PI * 2 * atanf((y - 8) / (x - 8)) + 5 * t);
            break;
        case 28:
            return sinf((t / 16) * i + x / y);
            break;
        case 29:
            return sinf(x / y - y / x + t);
            break;
        case 30:
            return sinf(t - sqrtf(x * x + y * y));
            break;
        case 31:
            return sinf(x + t) + sinf(y + t) + sinf(x + y + t) / 3;
            break;
        case 32:
            return 1 - hypotf(sinf(t) * 9 - x, cosf(t) * 9 - y) / 9;
            break;
        case 33:
            return 1 - fmod((x * x - y + t * (fmod(1 + x * x, 5.0f)) * 3.0f), 16.0f) / 16.0f;
            break;
        case 34:
            return sinf(sinf(y) + x + t) * cosf(t + y);
            break;
        case 35:
            return sinf(x + sinf(y / 2 + t * 5)) + cosf(y);
            break;
        case 36:
            return x > 7 - (sinf(t * 5 + y * 3) * 6) ? 1 : .2f;
            break; // Ldir's DNA )))
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

    void processFrame(float t, float x, float y)
    {
        EVERY_N_MILLISECONDS(30)
        {
            gHue++;
        }
        float i = (y * 16) + x;
        float frame = constrain(code(t, i, x, y), -1, 1) * 255;
        if (frame >= 0)
        {
            g()->leds[XY((int)x, (MATRIX_HEIGHT - 1 - (int)y))] = CHSV(gHue, 255, frame);
        } // change to XY(x, y) for non rotate display
        else
        {
            g()->leds[XY((int)x, (MATRIX_HEIGHT - 1 - (int)y))] = CHSV(gHue + 55, 255, -frame);
        } // change to XY(x, y) for non rotate display
    }

  public:
    PatternSMTixyLand() : LEDStripEffect(EFFECT_MATRIX_SMTIXY_LAND, "TixyLand")
    {
    }

    PatternSMTixyLand(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Label(int n)
    {
        static const int kLabelTimeoutMS = 500;
        String result = str_sprintf("Tixy %d", n);

        auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(g_ptrSystem->EffectManager().GetBaseGraphics());
        pMatrix->SetCaption(result, kLabelTimeoutMS); // Half a second.
    }

    void Draw() override
    {
        // some formulas is hardcoded and fps get down. this speedup it
        float t = millis() / 1000.0;
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            for (int y = 0; y < MATRIX_HEIGHT; y++)
            {
                processFrame(t, x, y);
            }
        }
        EVERY_N_SECONDS(4)
        {
            effect++;
            if (effect > 36)
                effect = 0;
            Label(effect);
        }
        Label(effect); // This will only last one frame. This is already too slow.
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }
};
