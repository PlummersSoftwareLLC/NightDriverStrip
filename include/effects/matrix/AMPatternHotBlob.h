#pragma once

class HotBlob : public LEDStripEffect
{
public:
    HotBlob() : LEDStripEffect(EFFECT_MATRIX_BOUNCE, "Hot Blob") {}

  virtual void Start() override
  {
    timings.master_speed = 0.01;    // speed ratios for the oscillators
    timings.ratio[0] = 0.1;         // higher values = faster transitions
    timings.ratio[1] = 0.03;
    timings.ratio[2] = 0.03;
    timings.ratio[3] = 0.03;

    timings.offset[1] = 10;
    timings.offset[2] = 20;
    timings.offset[3] = 30;

    calculate_oscillators(timings);     // get linear movers and oscillators going
  }

  run_default_oscillators();

  virtual void Draw() override
  {
  auto graphics = g();

  for (int x = 0; x < MATRIX_WIDTH; x++)
    for (int y = 0; y < MATRIX_HEIGHT; y++)

      animation.dist       = distance[x][y]
      animation.angle      = polar_theta[x][y];

      animation.scale_x    = 0.07 + move.directional[0]*0.002;
      animation.scale_y    = 0.07;

      animation.offset_y   = -move.linear[0];
      animation.offset_x   = 0;
      animation.offset_z   = 0;

      animation.z          = 0;
      animation.low_limit  = -1;
      float show1          = render_value(animation);

      animation.offset_y   = -move.linear[1];
      float show3          = render_value(animation);

      animation.offset_x   = show3/20;
      animation.offset_y   = -move.linear[0]/2 + show1/70;
      animation.low_limit  = 0;
      float show2          = render_value(animation);

      animation.offset_x   = show3/20;
      animation.offset_y   = -move.linear[0]/2 + show1/70;
      animation.z          = 100;
      float show4          = render_value(animation);

      float radius = 11;   // radius of a radial brightness filter
      float radial = (radius-animation.dist)/animation.dist;

      float linear = (y+1)/(num_y-1.f);
#if 1
      CRGB color;
      color.r   = radial  * show2;
      color.g   = linear * radial* 0.3* (show2-show4);

      g()->setPixel(x, y, color);
#else
      pixel.red   = radial  * show2;
      pixel.green   = linear * radial* 0.3* (show2-show4);


      pixel = rgb_sanity_check(pixel);
      leds[xy(x, y)] = CRGB(pixel.red, pixel.green, pixel.blue);
#endif

  // FastLED.show();
  //
  }
};
