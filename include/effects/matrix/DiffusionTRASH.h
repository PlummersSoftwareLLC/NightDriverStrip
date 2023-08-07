#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1685-strobe-and-diffusion
// Was originally drawn for a lamp, but I like it on a panel.
// The original coordinate system had 0,0 in the LL corner. We have 0,0 in UL.

#if SOUND
class PatternSMStrobeDiffusion : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMStrobeDiffusion : public LEDStripEffect
#endif
{
private:
	uint8_t hue, hue2;           // gradual hue shift or some other cyclic counter
	uint8_t deltaHue, deltaHue2; // a couple more of the same when you need a lot
	uint8_t step;                // some counter of frames or sequences of operations
  uint8_t noise3d[MATRIX_WIDTH][MATRIX_HEIGHT];     // two-layer mask or storage of properties in the size of the entire matrix
//  uint8_t Speed = 250;         // 1-255 is speed
//  uint8_t Scale = 70;          // 1-100 is something parameter
//  uint8_t FPSdelay;
//  const int LOW_DELAY = 0;

public:
  PatternSMStrobeDiffusion() :
#if SOUND
		BeatEffectBase(1.50, 0.05),
#endif
		LEDStripEffect(EFFECT_MATRIX_SMSTROBE_DIFFUSION, "Strobe Diffusion")
  {
  }

  PatternSMStrobeDiffusion(const JsonObjectConst& jsonObject) :
#if SOUND
		BeatEffectBase(1.50, 0.05),
#endif
		LEDStripEffect(jsonObject)
  {
  }

	virtual void Start() override
  {
//  	  FPSdelay = 25U; // LOW_DELAY;
//    hue2 = 1;
	    g()->Clear();
  }


// =========== Christmas Tree ===========
//             © SlingMaster
//           EFF_CHRISTMAS_TREE
//             Christmas Tree
//---------------------------------------
		void VirtualSnow() {
		  for (uint8_t x = 0U; x < MATRIX_WIDTH; x++) {
			    for (uint8_t y = MATRIX_HEIGHT ; y > 1; y--) {
		      noise3d[x][y] = noise3d[x][y - 1];
		      if (noise3d[x][y] > 0) {
		      	if (x > 63) x = 63;
		      	if (y > 31) y = 31;
		      	if (x < 0) x = 0;
		      	if (y < 0) y = 0;
		        // g()->drawPixel(x, y, CHSV(170, 5U, 127 + random8(128)));
		      }
		    }
		  }
		  uint8_t posX = random(MATRIX_WIDTH);
		  for (uint8_t x = 0U; x < MATRIX_WIDTH; x++) {
		    // randomly fill in the top row
		    if (posX == x) {
		      if (step % 3 == 0) {
		        noise3d[x][0/*MATRIX_HEIGHT - 1U*/] = 1;
		      } else {
		        noise3d[x][0/*MATRIX_HEIGHT - 1U*/]  = 0;
		      }
		    } else {
		      noise3d[x][0/*MATRIX_HEIGHT - 1U*/]  = 0;
		    }
		  }
	}

	// функция получения цвета пикселя в матрице по его координатам
	CRGB getPixColorXY(uint8_t x, uint8_t y)
	{
	  // Just don't think about what this does to prefetch and prediction...
	  return g()->leds[g()->xy(x, y)];
	}

  virtual void Draw() override
  {
		const uint8_t DELTA = 1U;         // Vertical Alignment
		uint8_t STEP = 2U;
//	  STEP = floor((255 - Speed) / 64) + 1U; // for strob
//	  if (Scale > 128) {
//	  EVERY_N_MILLIS(500) {
	    // diffusion ---
//	  EVERY_N_MILLIS(27) {
//	    blurScreen(beatsin8(3, 64, 80));
		  EVERY_N_MILLIS(100) {
		  	g()->BlurFrame(beatsin8(1, 15, 255));
		  }

//	  }
//	    FPSdelay = LOW_DELAY;
//	    STEP = 1U;
	    //if (Scale < 75) {
	    //  // chaos ---
	    //  FPSdelay = 30;
	    //  VirtualSnow();
	    // }
//	  }
//	  } else {
	    // Strobe
	//    if (Scale > 25) {
//	      g()->DimAll(200);
//	      FPSdelay = 30;
//	    } else {
//	      g()->DimAll(240);
//	      FPSdelay = 40;
//	    }
//	  }

	  EVERY_N_MILLIS(250) {
	      //g()->DimAll(240);
	  }

	  EVERY_N_MILLIS(150) {
	  	VirtualSnow();
	  }

	  const uint8_t rows = (MATRIX_HEIGHT + 1) / 3U;
  	deltaHue = floor(250 / 64) * 64; // WAT?
		bool dir = false;

	  for (uint8_t y = 0; y < rows; y++) {
		  if (dir) {
		      if ((step % STEP) == 0) {   // small layers
		        g()->drawPixel(MATRIX_WIDTH - 1, y * 3 + DELTA, CHSV(step, 255U, 255U ));
		      } else {
		        g()->drawPixel(MATRIX_WIDTH - 1, y * 3 + DELTA, CHSV(170U, 255U, 1U));
		      }
		    } else {
		      if ((step % STEP) == 0) {   // big layers
		        g()->drawPixel(0, y * 3 + DELTA, CHSV((step + deltaHue), 255U, 255U));
		      } else {
		        g()->drawPixel(0, y * 3 + DELTA, CHSV(0U, 255U, 0U));
		      }
	    }

    // Shift layers.
	    for (uint8_t x = 1U ; x < MATRIX_WIDTH; x++) {
	      if (dir) {
	        g()->drawPixel(x - 1, y * 3 + DELTA, getPixColorXY(x, y * 3 + DELTA));
	      } else {    // ==>
	        g()->drawPixel(MATRIX_WIDTH - x, y * 3 + DELTA, getPixColorXY(MATRIX_WIDTH - x - 1, y * 3 + DELTA));
	      }
	    }
		dir = !dir;
		}

	  if (hue2 == 1) {
	    step ++;
	    if (step >= 254) hue2 = 0;
	  } else {
	    step --;
	    if (step < 1) hue2 = 1;
	  }

//    Scale++;
  }
#if SOUND
	virtual void HandleBeat(bool bMajor, float elapsed, float span) override
	{

	}
      #endif


};
