/*
  ___        _            ___  ______ _____    _
 / _ \      (_)          / _ \ | ___ \_   _|  (_)
/ /_\ \_ __  _ _ __ ___ / /_\ \| |_/ / | |_ __ ___  __
|  _  | '_ \| | '_ ` _ \|  _  ||    /  | | '__| \ \/ /
| | | | | | | | | | | | | | | || |\ \  | | |  | |>  <
\_| |_/_| |_|_|_| |_| |_\_| |_/\_| \_| \_/_|  |_/_/\_\

by Stefan Petrick 2023.

High quality LED animations for your next project.

This is a Shader and 5D Coordinate Mapper made for realtime
rendering of generative animations & artistic dynamic visuals.

This is also a modular animation synthesizer with waveform
generators, oscillators, filters, modulators, noise generators,
compressors... and much more.

VO.42 beta version

This code is licenced under a Creative Commons Attribution
License CC BY-NC 3.0

*/

#include <vector>
#include <FastLED.h>

#define num_oscillators 10

struct render_parameters {

  // TODO float center_x = (num_x / 2) - 0.5;   // center of the matrix
  // TODO float center_y = (num_y / 2) - 0.5;
  float center_x = (999 / 2) - 0.5;   // center of the matrix
  float center_y = (999 / 2) - 0.5;
  float dist, angle;
  float scale_x = 0.1;                  // smaller values = zoom in
  float scale_y = 0.1;
  float scale_z = 0.1;
  float offset_x, offset_y, offset_z;
  float z;
  float low_limit  = 0;                 // getting contrast by highering the black point
  float high_limit = 1;
};

render_parameters animation;     // all animation parameters in one place
struct oscillators {

  float master_speed;            // global transition speed
  float offset[num_oscillators]; // oscillators can be shifted by a time offset
  float ratio[num_oscillators];  // speed ratios for the individual oscillators
};

oscillators timings;             // all speed settings in one place

struct modulators {

  float linear[num_oscillators];        // returns 0 to FLT_MAX
  float radial[num_oscillators];        // returns 0 to 2*PI
  float directional[num_oscillators];   // returns -1 to 1
  float noise_angle[num_oscillators];   // returns 0 to 2*PI
};

modulators move;   // all oscillator based movers and shifters at one place


static const byte pNoise[] = {   151,160,137,91,90, 15,131, 13,201,95,96,
53,194,233, 7,225,140,36,103,30,69,142, 8,99,37,240,21,10,23,190, 6,
148,247,120,234,75, 0,26,197,62,94,252,219,203,117, 35,11,32,57,177,
33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,
48,27,166, 77,146,158,231,83,111,229,122, 60,211,133,230,220,105,92,
41,55,46,245,40,244,102,143,54,65,25,63,161, 1,216,80,73,209,76,132,
187,208, 89, 18,169,200,196,135,130,116,188,159, 86,164,100,109,198,
173,186, 3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,
212,207,206, 59,227, 47,16,58,17,182,189, 28,42,223,183,170,213,119,
248,152,2,44,154,163,70,221,153,101,155,167,43,172, 9,129,22,39,253,
19,98,108,110,79,113,224,232,178,185,112,104,218,246, 97,228,251,34,
242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,
150,254,138,236,205, 93,222,114, 67,29,24, 72,243,141,128,195,78,66,
215,61,156,180
};

class ANIMartRIX {

public:

int num_x; // how many LEDs are in one row?
int num_y; // how many rows?

#define radial_filter_radius 23.0;      // on 32x32, use 11 for 16x16

bool  serpentine;

std::vector<std::vector<float>> polar_theta;        // look-up table for polar angles
std::vector<std::vector<float>> distance;           // look-up table for polar distances


float show1, show2, show3, show4, show5, show6, show7, show8, show9, show0;


ANIMartRIX(int w, int h,  bool serpentine) {
  this->init(w, h, serpentine);
}

void init(int w, int h,  bool serpentine) {
  this->num_x  = w;
  this->num_y = h;
  this->serpentine = serpentine;
  render_polar_lookup_table((num_x / 2) - 0.5, (num_y / 2) - 0.5);          // precalculate all polar coordinates
                                                                           // polar origin is set to matrix centre
}

void calculate_oscillators(oscillators &timings) {

  double runtime = millis() * timings.master_speed;  // global anaimation speed

  for (int i = 0; i < num_oscillators; i++) {

    move.linear[i]      = (runtime + timings.offset[i]) * timings.ratio[i];     // continously rising offsets, returns              0 to max_float

    move.radial[i]      = fmodf(move.linear[i], 2 * PI);                        // angle offsets for continous rotation, returns    0 to 2 * PI

    move.directional[i] = sinf(move.radial[i]);                                 // directional offsets or factors, returns         -1 to 1

    move.noise_angle[i] = PI * (1 + pnoise(move.linear[i], 0, 0));              // noise based angle offset, returns                0 to 2 * PI

  }
}
void run_default_oscillators(){

  timings.master_speed = 0.005;    // master speed

  timings.ratio[0] = 1;           // speed ratios for the oscillators, higher values = faster transitions
  timings.ratio[1] = 2;
  timings.ratio[2] = 3;
  timings.ratio[3] = 4;
  timings.ratio[4] = 5;
  timings.ratio[5] = 6;
  timings.ratio[6] = 7;
  timings.ratio[7] = 8;
  timings.ratio[8] = 9;
  timings.ratio[9] = 10;


  timings.offset[0] = 000;
  timings.offset[1] = 100;
  timings.offset[2] = 200;
  timings.offset[3] = 300;
  timings.offset[4] = 400;
  timings.offset[5] = 500;
  timings.offset[6] = 600;
  timings.offset[7] = 700;
  timings.offset[8] = 800;
  timings.offset[9] = 900;

  calculate_oscillators(timings);
}

// Convert the 2 polar coordinates back to cartesian ones & also apply all 3d transitions.
// Calculate the noise value at this point based on the 5 dimensional manipulation of
// the underlaying coordinates.

float render_value(render_parameters &animation) {

  // convert polar coordinates back to cartesian ones

  float newx = (animation.offset_x + animation.center_x - (cosf(animation.angle) * animation.dist)) * animation.scale_x;
  float newy = (animation.offset_y + animation.center_y - (sinf(animation.angle) * animation.dist)) * animation.scale_y;
  float newz = (animation.offset_z + animation.z) * animation.scale_z;

  // render noisevalue at this new cartesian point

  float raw_noise_field_value = pnoise(newx, newy, newz);

  // A) enhance histogram (improve contrast) by setting the black and white point (low & high_limit)
  // B) scale the result to a 0-255 range (assuming you want 8 bit color depth per rgb chanel)
  // Here happens the contrast boosting & the brightness mapping

  if (raw_noise_field_value < animation.low_limit)  raw_noise_field_value =  animation.low_limit;
  if (raw_noise_field_value > animation.high_limit) raw_noise_field_value = animation.high_limit;

  float scaled_noise_value = map_float(raw_noise_field_value, animation.low_limit, animation.high_limit, 0, 255);

  return scaled_noise_value;
}


// given a static polar origin we can precalculate
// the polar coordinates

void render_polar_lookup_table(float cx, float cy) {

  polar_theta.resize(num_x, std::vector<float>(num_y, 0.0f));
  distance.resize(num_x, std::vector<float>(num_y, 0.0f));

  for (int xx = 0; xx < num_x; xx++) {
    for (int yy = 0; yy < num_y; yy++) {

      float dx = xx - cx;
      float dy = yy - cy;

      distance[xx][yy]    = hypotf(dx, dy);
      polar_theta[xx][yy] = atan2f(dy, dx);
    }
  }
}

// find the right led index according to you LED matrix wiring

uint16_t xy(uint8_t x, uint8_t y) {
  if (serpentine &&  y & 1)                             // check last bit
    return (y + 1) * num_x - 1 - x;      // reverse every second line for a serpentine lled layout
  else
    return y * num_x + x;                // use this equation only for a line by line layout
}                                        // remove the previous 3 lines of code in this case


};
