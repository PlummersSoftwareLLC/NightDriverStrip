#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"
#include "gfxbase.h"

// Derived from https://editor.soulmatelights.com/gallery/2128-bluringcolors

class PatternSMBlurringColors : public LEDStripEffect
{
  private:
    // A more cache-friendly version of the 7 independent arrays that were
    // used. This allows better locality per member, especially since we
    // access them sequentially.
    // Nothing special here; just a default-zero initialized struct.
    class PowderItem
    {
      public:
        PowderItem()
        {
        }

        void Clear()
        {
            _position_x = 0.0f;
            _position_y = 0.0f;
            _speed_x = 0.0f;
            _speed_y = 0.0f;
            _state = 0;
            _hue = 0;
            _is_shift = false;
        }

        float _position_x;
        float _position_y;
        float _speed_x;
        float _speed_y;
        uint8_t _hue;
        uint8_t _state;
        bool _is_shift;
    };

    uint8_t Scale = 10; // 1-100 # of Boids. Should be a Setting
    uint8_t Speed = 95; // 1-100. Odd or even only. Should be a bool Setting

    const int WIDTH = MATRIX_WIDTH;
    const int HEIGHT = MATRIX_HEIGHT;

    static constexpr int powder_item_max_count = (100U);
    std::array<PowderItem, powder_item_max_count> _powder_items;

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

    uint8_t enlargedObjectNUM; // number of objects used in the effect

    uint8_t hue, hue2;  // gradual shift in hue or some other cyclic counter
    uint8_t deltaValue; // just a reusable variable
    uint8_t step;       // some kind of frame or sequence counter

    [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const
    {
        return g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)];
    }

    void drawPixelXY(uint8_t x, uint8_t y, CRGB color)
    {
        y = MATRIX_HEIGHT - 1 - y;
        if (g()->isValidPixel(x, y)) 
            g()->leds[XY(x, y)] = color;
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color)
    {
        // Extract the fractional parts and derive their inverses.
        uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        // Calculate the intensities for each affected pixel.
        uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        // Multiply the intensities by the colour, and saturating-add them
        // to the pixels.
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

    bool inline ParticlesUpdate(PowderItem& powder_item)
    {
        powder_item._state--; // ttl // You also need to add speedfactor here.
                                  // good luck there!

        // Apply velocity.
        powder_item._position_x += powder_item._speed_x;
        powder_item._position_y += powder_item._speed_y;
        if (powder_item._state == 0 ||
            powder_item._position_x <= -1 || powder_item._position_x >= WIDTH ||
            powder_item._position_y <= -1 || powder_item._position_y >= HEIGHT)
            powder_item._is_shift = false;

        return powder_item._is_shift;
    }

    // ============= EFFECT SOURCES ===============
    // (c) SottNick
    // Such a spectacle

    void FountainsDrift(uint8_t j)
    {
        Boid&boid = boids[j];
        boid.location.x += boid.velocity.x;
        boid.location.y += boid.velocity.y;
        if (boid.location.x + boid.velocity.x < 0)
        {
            // boid.location.x = WIDTH - 1 + boid.location.x;
            boid.location.x = -boid.location.x;
            boid.velocity.x = -boid.velocity.x;
        }
        if (boid.location.x > WIDTH - 1)
        {
            // boid.location.x = boid.location.x + 1 - WIDTH;
            boid.location.x = WIDTH + WIDTH - 2 - boid.location.x;
            boid.velocity.x = -boid.velocity.x;
        }
        if (boid.location.y < 0)
        {
            // boid.location.y = HEIGHT - 1 + boid.location.y;
            boid.location.y = -boid.location.y;
            boid.velocity.y = -boid.velocity.y;
        }
        if (boid.location.y > HEIGHT - 1)
        {
            // boid.location.y = boid.location.y + 1 - HEIGHT;
            boid.location.y = HEIGHT + HEIGHT - 2 - boid.location.y;
            boid.velocity.y = -boid.velocity.y;
        }
    }

    void FountainsEmit(PowderItem& powder_item)
    {
        if (hue++ & 0x01)
            hue2 += 4;

        uint8_t j = random8(enlargedObjectNUM);
        FountainsDrift(j);
        powder_item._position_x = boids[j].location.x;
        powder_item._position_y = boids[j].location.y;

        powder_item._speed_x = (random8() - 127.f) / 512.f;
        powder_item._speed_y =
            sqrtf(0.0626f - powder_item._speed_x * powder_item._speed_x);

        const auto kScalingFactor = 1.25f;
        powder_item._speed_x *= kScalingFactor;
        powder_item._speed_y *= kScalingFactor;

        if (random8(2U))
        {
            powder_item._speed_y = -powder_item._speed_y;
        }
        powder_item._state = random8(50, 250);

        if (Speed & 0x01)
            powder_item._hue = hue2;
        else
            powder_item._hue = boids[j].colorIndex;
        powder_item._is_shift = true; // particle->isAlive
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

        enlargedObjectNUM = (Scale + 5U); // / 99.0 * (AVAILABLE_BOID_COUNT) ;
        if (enlargedObjectNUM > AVAILABLE_BOID_COUNT)
            enlargedObjectNUM = AVAILABLE_BOID_COUNT;

        deltaValue = powder_item_max_count /
            (sqrt(CENTER_X_MAJOR * CENTER_X_MAJOR + CENTER_Y_MAJOR * CENTER_Y_MAJOR) * 4U) + 1U;
            // 4 - this is because in 1 cycle the particle flies exactly a
            // quarter the distance between 2 neighboring pixels

        for (auto& powder_item : _powder_items)
            powder_item.Clear();

        for (int j = 0; j < enlargedObjectNUM; j++)
        {
            auto boid = Boid(random8(WIDTH), random8(HEIGHT));
            boid.velocity.x = 1;
            boid.velocity.y = 1;

            boids[j] = boid;
        }
    }

    void Draw() override
    {
        step = deltaValue; // counter of the number of particles in the
                           // queue for nucleation in this loop
        g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, 0, 27);

        // go over particles and update matrix cells on the way
        for (auto& powder_item : _powder_items)
        {
            if (!powder_item._is_shift && step)
            {
                FountainsEmit(powder_item);
                step--;
            }

            if (powder_item._is_shift && ParticlesUpdate(powder_item)) {

                // generate RGB values for particle
                CRGB baseRGB = CHSV(powder_item._hue, 255, 255);

                baseRGB.nscale8(powder_item._state); // equivalent
                drawPixelXYF(powder_item._position_x, powder_item._position_y, baseRGB);
            }
        }
    }
};
