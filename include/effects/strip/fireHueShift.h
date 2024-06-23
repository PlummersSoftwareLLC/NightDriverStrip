#include "Arduino.h"
#define FASTLED_INTERNAL
#include "FastLED.h"
//#include "utilityfx.h"


//CRGB HeatColor( uint8_t temperature);
//CRGB HueHeatColor( uint8_t temperature, CHSV color1, CHSV color2, bool clockwise);
CRGB HueHeatColor( uint8_t temperature, CHSV color1, CHSV color2, byte gradPoint, CHSV color3, bool clockwise);
class FireHueEffect
{
    public:
        struct CRGB * _LED_Array;
 //       byte * _Heat_map;
        int _NUM_LEDS;               // How many pixels the flame is total
        // Unused int FlameType;          // 0 -> Classic
        CHSV Color1;
        CHSV Color2;
        byte GradPoint;
        CHSV Color3;
        bool Clockwise;
        int Cooling;            // Rate at which the pixels cool off
        int Sparks;             // How many sparks will be attempted each frame
        int SparkHeight;        // If created, max height for a spark
        int Sparking;           // Probability of spark each attempt
        bool bReversed;         // If reversed, we draw from 0 outwards
        bool bMirrored;         // If mirrored we split and duplicate the drawing

        byte * heat;

        static const byte BlendSelf = 2;
        static const byte BlendNeighbor1 = 3;
        static const byte BlendNeighbor2 = 2;
        static const byte BlendNeighbor3 = 1;
        static const byte BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);

        FireHueEffect(struct CRGB * LED_Array, int NUM_LEDS, CHSV color1, CHSV color2, byte gradPoint, CHSV color3, bool clockwise, int cooling = 80, int sparking = 50, int sparks = 3, int sparkHeight = 4, bool breversed = true, bool bmirrored = false)//, byte * Heat_map)
            : 
              _LED_Array(LED_Array),
              _NUM_LEDS(NUM_LEDS),
              //FlameType(flametype),
              Color1(color1),
              Color2(color2),
              GradPoint(gradPoint),
              Color3(color3),
              Clockwise(clockwise),
              Cooling(cooling),
              Sparks(sparks),
              SparkHeight(sparkHeight),
              Sparking(sparking),
              bReversed(breversed),
              bMirrored(bmirrored)
//              {}

//              bMirrored(bmirrored)
        {
            if (bMirrored)
                _NUM_LEDS = _NUM_LEDS / 2;
            heat = new byte[_NUM_LEDS] { 0 };

        }

//        virtual ~FireEffect()
//        {
//            delete [] heat;
//        }

    void DrawFire() {
                //First cool each cell by a little bit
        for(int i = 0; i < _NUM_LEDS; i++)
            heat[i] = max(0L, heat[i] - random(0, ((Cooling*10)/_NUM_LEDS)+2));

        //Next drift heat up and diffuse it a little bit
        for (int i = 0; i < _NUM_LEDS; i++)
            heat[i] = (heat[i] * BlendSelf +
                        heat[(i+1) % _NUM_LEDS] * BlendNeighbor1 +
                        heat[(i+2) % _NUM_LEDS] * BlendNeighbor2 +
                        heat[(i+3) % _NUM_LEDS] * BlendNeighbor3)
                        / BlendTotal;

        // Randomly ignite new sparks down in the flame
        for (int i = 0; i < Sparks; i++)
        {
            if (random(255) < Sparking)
            {
                int y = _NUM_LEDS - 1 - random(SparkHeight);

                heat[y] = 250;
            }
        }

        // Finally, convert heat to a color
        for (int i = 0; i < _NUM_LEDS; i++)
        {
            CRGB color = HueHeatColor(heat[i], Color1, Color2, GradPoint, Color3, Clockwise);
            int j = bReversed ? (_NUM_LEDS - 1 - i) : i;
            DrawPixelsCRGBArray(_LED_Array, _NUM_LEDS, j, 1, color);
            if (bMirrored)
                DrawPixelsCRGBArray(_LED_Array, 2*_NUM_LEDS,!bReversed ? (2 * _NUM_LEDS - 1 - i) : _NUM_LEDS + i, 1, color);
        }
    }

    void DrawPixelsCRGBArray(struct CRGB * LED_Array, int NUM_LEDS, float fPos, float count, CRGB color) {
        // Calculate how much the first pixel will hold
        float availFirstPixel = 1.0f - (fPos - (long)(fPos));
        float amtFirstPixel = min(availFirstPixel, count);
        float remaining = min(count, NUM_LEDS-fPos);
        int iPos = fPos;

        // Blend (add) in the color of the first partial pixel

        if (remaining > 0.0f)
        {
            LED_Array[iPos++] += ColorFraction(color, amtFirstPixel);
            remaining -= amtFirstPixel;
        }

        // Now draw any full pixels in the middle
        while (remaining > 1.0f)
        {
            LED_Array[iPos++] += color;
            remaining--;
        }

        // Draw tail pixel, up to a single full pixel

        if(remaining > 0.0f)
        {
            LED_Array[iPos] += ColorFraction(color, remaining);
        }


    }
};

CRGB HueHeatColor( uint8_t temperature, CHSV color1, CHSV color2, byte gradPoint, CHSV color3, bool clockwise)
{
    CRGB heatcolor;
    byte hue;
    byte sat;
    byte val;
    int16_t temp = temperature;
    if (temp < 8)
        return heatcolor = CRGB(0,0,0);
    
    //HOT TEMP, COLOR 1-2
    if (temp > gradPoint)
    {
        int16_t huedelta = uint8_t(color1.hue - color2.hue);//fast modulo 256, at least that's what's intended
        if (clockwise == true) {
            huedelta-=256;
        }
        hue = ((temp-gradPoint)*huedelta)/(256-gradPoint)+color2.hue;
        sat = ((temp-gradPoint)*(color1.sat-color2.sat))/(256-gradPoint)+color2.sat;
        val = ((temp-gradPoint)*(color1.val-color2.val))/(256-gradPoint)+color2.val;
        //heatcolor = CHSV(hue,sat,val);
    }
    //COLD TEMP, COLOR 2-3
    else //(8 < temperature < GradPoint)
    {
        int16_t huedelta = uint8_t(color2.hue - color3.hue);//fast modulo 256, at least that's what's intended
        if (clockwise == true) {
            huedelta-=256;
        }
        hue = (temperature*huedelta)/gradPoint+color3.hue;
        sat = (temperature*(color2.sat-color3.sat))/gradPoint+color3.sat;
        val = (temperature*(color2.val-color3.val))/gradPoint+color3.val;
        //heatcolor = CHSV(hue,sat,val);
    }
    return heatcolor = CHSV(hue,sat,val);
}
