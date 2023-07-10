#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"
#include <algorithm>
#include <memory>
#include <vector>

class Ball
{
private:
    float vx_, vy_; // Velocity of movement along each axis.
    CRGB color_; // Private because I'd considered mutating it while drawing.

public:
    float x_, y_; // Getters and setters are only annoying here.
		  //
    Ball(uint8_t x, uint8_t y, float vx, float vy) {
        x_ = x;
        y_ = y;
        vx_ = vx;
        vy_ = vy;
    }

    void SetColor(CRGB color)
    {
	color_ = color;
    }

    CRGB GetColor() const
    {
	return color_;
    }

    uint16_t GetColorRGB565() const
    {
        return ((color_.r >> 3) << 11) |
	    ((color_.g>>2) << 5) |
	    color_.b >> 3;
    }


    // Move, change ball directions at edges, and clamp.
    void Update() {
        x_ = x_ + vx_;
	y_ = y_ + vy_;

        if(x_ < 0 || x_ >= MATRIX_WIDTH) {
            vx_ = -vx_;
            x_ = x_ + vx_;
        }
	if(y_ < 0 || y_ >= MATRIX_HEIGHT) {
            vy_ = -vy_;
            y_ = y_ + vy_;
        }
    }
};

class PatternBalls : public BeatEffectBase, public LEDStripEffect
{
private:
    std::vector<std::unique_ptr<Ball>> balls;

    // These might eventually change at runtime or via settings.
    const int kInitialBalls = 4;
    const int kFadeEveryThisMS = 50;
    const int kPercentToFade = 60;
    const int kChangeBurstRadius = 2; // Center pixel + radius on each edge.

public:
    PatternBalls() :
	BeatEffectBase(1.50, 0.05),
	LEDStripEffect(EFFECT_MATRIX_BALLS, "Balls")
    {
    }

    PatternBalls(const JsonObjectConst& jsonObject) :
	BeatEffectBase(1.50, 0.05),
	LEDStripEffect(jsonObject)
    {
    }

    virtual void Start() override
    {
	balls.resize(kInitialBalls);
	for (int i = 0 ; i < balls.size(); i++) {
	    // Visually tuned (made up) adjustments to the increment to
	    // get multiple balls going at different velocities and directions.
	    float inc = .5 + i * .17;
	    float inc2 = .6 + i * .31;

	    balls[i] = std::make_unique<Ball>(1+ i, 1 + i, -inc, inc2);
	    balls[i]->SetColor(RandomRainbowColor());
	}
    }

    virtual void Draw() override
    {
        ProcessAudio(); // Without this, HandleBeat has no audio to process.

	for (auto&& b : balls) {
	    b->Update();
	    g()->leds[g()->xy(b->x_, b->y_)] = b->GetColor();

#if 0
debugI("Beat: %p %p %u %u", &balls, b.get(),  (uint16_t)b->x_, (uint16_t)b->y_);
	    g()->fillCircle((uint16_t)b->x_, (uint16_t)b->y_,
		             1/*4 + (random8() & 0x7)*/, random16());
	    g()->fillCircle((uint16_t)MATRIX_WIDTH-3, (uint16_t)MATRIX_HEIGHT-3,
		             2/*4 + (random8() & 0x7)*/, random16());
#endif
	}

        EVERY_N_MILLISECONDS(kFadeEveryThisMS) {
	     fadeToBlackBy(g()->leds, NUM_LEDS, kPercentToFade);
	}

    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
	// debugI("Beat: %u %f %f\n", bMajor, elapsed, span);
	if (elapsed < .2f) return; // short beat? We may already be in a draw.

	for (auto&& b : balls) {
	    b->SetColor(RandomRainbowColor());
	    // Serious impedance mismatch between the FastLED layer that
	    // takes floating point coords and 24-bit colors and the
	    // Arduino drawing layer which wants ints and 16-bit RGB565.
	    // Because this is "under" FastLED, I wonder if it knows about
	    // wrapping NeoPixel style matrices. This def works on HUB75.
	    //
	    // Worse still, because it's lower than FastLED, it doesn't
	    // know the sizeof the added LED array, so if you attempt 
	    // to draw a circle that's nearthe beginning or end, it will
	    // dutifully draw that circle ... outside of the LED array.
	    // This eventually causes heap corruption and a crash. Thus,
	    // we manually clamp the clipping region of the circle so we
	    // draw only inside the leds[] boundaries.
	    int16_t r = kChangeBurstRadius;
	    int x = b->x_;
	    int y = b->y_;
	    x = std::clamp(x,
		    kChangeBurstRadius + r + 0, MATRIX_WIDTH - r - 1);
	    y = std::clamp(y,
		    kChangeBurstRadius + r + 0, MATRIX_HEIGHT - r - 1);
	    g()->fillCircle(x, y, r, random16());
	}
    }
};
