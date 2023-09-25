#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

// Derived from https://editor.soulmatelights.com/gallery/2128-bluringcolors

class PatternSMBlurringColors : public LEDStripEffect
{
  private:
    uint8_t Scale = 10; // 1-100 Setting
    uint8_t Speed = 95; // 1-100. Odd or even only. Setting

    const int WIDTH = MATRIX_WIDTH;
    const int HEIGHT = MATRIX_HEIGHT;

    // matrix size constants are calculated only here and do not change in effects
    const uint8_t CENTER_X_MINOR =
        (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01); // the center of the matrix according to ICSU, shifted to the
                                                          // smaller side, if the width is even
    const uint8_t CENTER_Y_MINOR =
        (MATRIX_HEIGHT / 2) -
        ((MATRIX_HEIGHT - 1) & 0x01); // center of the YGREK matrix, shifted down if the height is even
    const uint8_t CENTER_X_MAJOR =
        MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2); // the center of the matrix according to IKSU,
                                               // shifted to a larger side, if the width is even
    const uint8_t CENTER_Y_MAJOR =
        MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2); // center of the YGREK matrix, shifted up if the height is even

    // FIXME: this is large enough it should probably go into PSMEM.
    static constexpr int trackingOBJECT_MAX_COUNT = (100U);
    // максимальное количество отслеживаемых объектов (очень влияет на
    // расход памяти)
    float trackingObjectPosX[trackingOBJECT_MAX_COUNT];
    float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
    float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
    float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
    uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];
    uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];
    bool trackingObjectIsShift[trackingOBJECT_MAX_COUNT];
    // BugBug: could be a std::Bitfield<T>
    static constexpr int enlargedOBJECT_MAX_COUNT = (MATRIX_WIDTH * 2);
    // максимальное количество сложных отслеживаемых объектов
    // (меньше, чем trackingOBJECT_MAX_COUNT)
    uint8_t enlargedObjectNUM; // используемое в эффекте количество объектов

    float accel;
    uint8_t hue, hue2;  // постепенный сдвиг оттенка или какой-нибудь другой
                        // цикличный счётчик
    uint8_t deltaValue; // просто повторно используемая переменная
    uint8_t step; // какой-нибудь счётчик кадров или последовательностей операций

    [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const
    {
        return g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)];
    }

    void drawPixelXY(uint8_t x, uint8_t y, CRGB color)
    {
	y = MATRIX_HEIGHT - 1 - y;
        if (g()->isValidPixel(x, y)) {
            uint32_t thisPixel = XY(x, y);
            g()->leds[thisPixel] = color;
	}
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color)
    {
        //  if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть
        //  как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
        // extract the fractional parts and derive their inverses
        uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        // calculate the intensities for each affected pixel
        // #define WU_WEIGHT(a,b) ((uint8_t) (((a)*(b)+(a)+(b))>>8))
        uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        // multiply the intensities by the colour, and saturating-add them to the
        // pixels
        for (uint8_t i = 0; i < 4; i++)
        {
            int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
            if (g()->isValidPixel(xn, yn) == false)
                continue;
            CRGB clr = getPixColorXY(xn, yn);
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            drawPixelXY(xn, yn, clr);
        }
    }

    static const uint8_t AVAILABLE_BOID_COUNT = 7U;
    Boid boids[AVAILABLE_BOID_COUNT];
    void particlesUpdate2(uint8_t i)
    {
        trackingObjectState[i]--; // ttl // ещё и сюда надо speedfactor вкорячить.
                                  // удачи там!

        // apply velocity
        trackingObjectPosX[i] += trackingObjectSpeedX[i] + accel;
        trackingObjectPosY[i] += trackingObjectSpeedY[i] + accel;
        if (trackingObjectState[i] == 0 || trackingObjectPosX[i] <= -1 || trackingObjectPosX[i] >= WIDTH ||
            trackingObjectPosY[i] <= -1 || trackingObjectPosY[i] >= HEIGHT)
            trackingObjectIsShift[i] = false;
    }

    // ============= ЭФФЕКТ ИСТОЧНИКИ ===============
    // (c) SottNick
    // такое себе зрелище

    void fountainsDrift(uint8_t j)
    {
        // float shift = random8()
        boids[j].location.x += boids[j].velocity.x + accel;
        boids[j].location.y += boids[j].velocity.y + accel;
        if (boids[j].location.x + boids[j].velocity.x < 0)
        {
            // boids[j].location.x = WIDTH - 1 + boids[j].location.x;
            boids[j].location.x = -boids[j].location.x;
            boids[j].velocity.x = -boids[j].velocity.x;
        }
        if (boids[j].location.x > WIDTH - 1)
        {
            // boids[j].location.x = boids[j].location.x + 1 - WIDTH;
            boids[j].location.x = WIDTH + WIDTH - 2 - boids[j].location.x;
            boids[j].velocity.x = -boids[j].velocity.x;
        }
        if (boids[j].location.y < 0)
        {
            // boids[j].location.y = HEIGHT - 1 + boids[j].location.y;
            boids[j].location.y = -boids[j].location.y;
            boids[j].velocity.y = -boids[j].velocity.y;
        }
        if (boids[j].location.y > HEIGHT - 1)
        {
            // boids[j].location.y = boids[j].location.y + 1 - HEIGHT;
            boids[j].location.y = HEIGHT + HEIGHT - 2 - boids[j].location.y;
            boids[j].velocity.y = -boids[j].velocity.y;
        }
    }

    void fountainsEmit(uint8_t i)
    {
        if (hue++ & 0x01)
            hue2 += 4;

        uint8_t j = random8(enlargedObjectNUM);
        fountainsDrift(j);
        trackingObjectPosX[i] = boids[j].location.x;
        trackingObjectPosY[i] = boids[j].location.y;

        trackingObjectSpeedX[i] = ((float)random8() - 127.) / 512.; // random(_hVar)-_constVel; // particle->vx
        trackingObjectSpeedY[i] =
            sqrt(0.0626 - trackingObjectSpeedX[i] *
                              trackingObjectSpeedX[i]); // sqrt(pow(_constVel,2)-pow(trackingObjectSpeedX[i],2));
                                                        // // particle->vy зависит от

        const auto kScalingFactor = 1.25;
        trackingObjectSpeedX[i] *= kScalingFactor;
        trackingObjectSpeedY[i] *= kScalingFactor;

        if (random8(2U))
        {
            trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
        }
        trackingObjectState[i] = random8(50, 250); // random8(minLife, maxLife);// particle->ttl
        if (Speed & 0x01)
            trackingObjectHue[i] = hue2; // (counter/2)%255; // particle->hue
        else
            trackingObjectHue[i] = boids[j].colorIndex; // random8();
        trackingObjectIsShift[i] = true;                // particle->isAlive
    }

  public:
    PatternSMBlurringColors() : LEDStripEffect(EFFECT_MATRIX_SMBLURRING_COLORS, "Powder")
    {
    }

    PatternSMBlurringColors(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();

        enlargedObjectNUM = (Scale + 5U); // / 99.0 * (AVAILABLE_BOID_COUNT ) ;
        if (enlargedObjectNUM > AVAILABLE_BOID_COUNT)
            enlargedObjectNUM = AVAILABLE_BOID_COUNT;

        // deltaValue = 10; // количество зарождающихся частиц за 1 цикл
        deltaValue =
            trackingOBJECT_MAX_COUNT / (sqrt(CENTER_X_MAJOR * CENTER_X_MAJOR + CENTER_Y_MAJOR * CENTER_Y_MAJOR) * 4U) +
            1U; // 4 - это потому что за 1 цикл частица пролетает ровно четверть
                // расстояния между 2мя соседними пикселями

        for (int i = 0; i < trackingOBJECT_MAX_COUNT; i++)
            trackingObjectIsShift[i] = false;

        for (int j = 0; j < enlargedObjectNUM; j++)
        {
            boids[j] = Boid(random8(WIDTH), random8(HEIGHT));
            // boids[j].location.x = random8(WIDTH);
            // boids[j].location.y = random8(HEIGHT);
            boids[j].velocity.x = 1; //((float)random8()-127.)/512.;
            boids[j].velocity.y = 1; // sqrt(0.0626-boids[j].velocity.x*boids[j].velocity.x) /  8.; //
                                     // скорость источников в восемь раз ниже, чем скорость частиц
            // boids[j].velocity.x /= 8.; // скорость источников в восемь раз ниже,
            // чем скорость частиц if(random8(10U))
            //   boids[j].velocity.y = -boids[j].velocity.y;
            // boids[j].colorIndex += random8();
        }
    }

    void Draw() override
    {
        step = deltaValue; //счётчик количества частиц в очереди на зарождение в
                           //этом цикле
        // dimAll(10);
        g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, 0, 27);
        // go over particles and update matrix cells on the way
        for (int i = 0; i < trackingOBJECT_MAX_COUNT; i++)
        {
            if (!trackingObjectIsShift[i] && step)
            {
                // emitter->emit(&particles[i], this->g);
                fountainsEmit(i);
                step--;
            }
            if (trackingObjectIsShift[i])
            { // particle->isAlive
                // particles[i].update(this->g);
                particlesUpdate2(i);

                // generate RGB values for particle
                CRGB baseRGB = CHSV(trackingObjectHue[i], 255, 255); // particles[i].hue

                // baseRGB.fadeToBlackBy(255-trackingObjectState[i]);
                baseRGB.nscale8(trackingObjectState[i]); // эквивалент
                drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], baseRGB);
            }
        }
    }
};
