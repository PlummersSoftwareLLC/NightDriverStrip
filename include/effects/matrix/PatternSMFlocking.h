#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

// Inspired by https://editor.soulmatelights.com/gallery/2254-flocking
// Almost a textbook Boid demo: Separation, Cohesion, Alignment.

class PatternSMFlocking : public LEDStripEffect
{
  private:
    // Just about enough time to spread after a collision on a 64x32
    // Too many and the flock is to large and you spend all the time off
    // screen.
    static constexpr int NUM_PARTICLES = 10;
    Boid boids[NUM_PARTICLES];

    [[nodiscard]] CRGB inline getPixColorXY(uint8_t x, uint8_t y) const
    {
        // Mesmerizer flips the Y axis here.
        y = MATRIX_HEIGHT - 1 - y;

        if (g()->isValidPixel(x, y))
        {
            return g()->leds[XY(x, y)];
        }
        return 0;
    }

    void inline drawPixelXY(uint8_t x, uint8_t y, CRGB color) const
    {
        // Mesmerizer flips the Y axis here.
        y = MATRIX_HEIGHT - 1 - y;

        assert(g()->isValidPixel(x, y));
        g()->leds[XY(x,y)] = color;
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color) //, uint8_t darklevel = 0U) const
    {
        //  if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть
        //  как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
        // extract the fractional parts and derive their inverses
        uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        // calculate the intensities for each affected pixel
        // #define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
        uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        // multiply the intensities by the colour, and saturating-add them to the
        // pixels
        for (uint8_t i = 0; i < 4; i++)
        {
            int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
            CRGB clr = getPixColorXY(xn, yn);
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            drawPixelXY(xn, yn, clr);
        }
    }

  public:
    PatternSMFlocking() : LEDStripEffect(EFFECT_MATRIX_SMFLOCKING, "Flocking")
    {
    }

    PatternSMFlocking(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            boids[i] = Boid(random(MATRIX_WIDTH-1), random(MATRIX_HEIGHT-1));
            // The default is too fast; we spend all our time bouncing off the walls.
            boids[i].maxspeed = 0.7;
        }
    }

    void Draw() override
    {
	for (int i = 0; i < NUM_LEDS; i++)
		g()->leds[i] = CRGB::Red;
	return;
        // Run the entire queue. The kink is that update() may move adjacent boids as it'll
        // shuffle them in a flock. Attempting to contrain it onto screen inside this pass
        // is therefore futile. We make another run over the list to do the actual clamp and
        // draw. The update() on boid[N] might push boid[N-1] out of bounds.
        for (auto &boid : boids)
        {
            boid.update(boids, NUM_PARTICLES); // Let update() move the whole flock.
            boid.avoidBorders(); // Keep them 8 pixels from edges.
            boid.bounceOffBorders(.7); // Bounce anything that update() DOES put over the edge.
        }
        for (auto &boid : boids)
        {
            // This drawPixel draws a blended 2x2 block, so we have to constrain even more.
            boid.location.x = std::clamp(boid.location.x, 1.0f, (float) MATRIX_WIDTH - 2);
            boid.location.y = std::clamp(boid.location.y, 1.0f, (float) MATRIX_HEIGHT - 2);
            drawPixelXYF(boid.location.x, boid.location.y,
                         ColorFromPalette(PartyColors_p, boid.hue * 15, 255, LINEARBLEND));
        }
        fadeAllChannelsToBlackBy(25);
    }
};
