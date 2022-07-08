#ifndef PatternSub_H
#define PatternSub_H


#include "globals.h"
#include "ledstripeffect.h"
#include "gfxbase.h"
#include "ledmatrixgfx.h"

class PatternSubscribers : public LEDStripEffect
{
  private:

  public:
  
  PatternSubscribers() : LEDStripEffect("Subscribers")
  {
  }

  static volatile long cSubscribers;
  static volatile long cViews;
  static char szChannelID[];
  static char szChannelName1[];

  virtual void Draw()
  {
      char szBuffer[32];

      LEDMatrixGFX::backgroundLayer.fillScreen(rgb24(0, 16, 64));
      LEDMatrixGFX::backgroundLayer.setFont(font5x7);

      // Draw a border around the edge of the panel
      LEDMatrixGFX::backgroundLayer.drawRectangle(0, 1, MATRIX_WIDTH-1, MATRIX_HEIGHT-2, rgb24(160,160,255));
      
      // Draw the channel name
      LEDMatrixGFX::backgroundLayer.drawString(2, 3, rgb24(255,255,255), szChannelName1);

      // Start in the middle of the panel and then back up a half a row to center vertically,
      // then back up left one half a char for every 10s digit in the subscriber count.  This
      // shoud center the number on the screen

      const int CHAR_WIDTH = 6;
      const int CHAR_HEIGHT = 7;
      int x = MATRIX_WIDTH / 2 - CHAR_WIDTH / 2;
      int y = MATRIX_HEIGHT / 2 - CHAR_HEIGHT / 2;
      int z = cSubscribers;

      while (z/=10)
        x-= CHAR_WIDTH / 2;

      sprintf(szBuffer, "%ld", cSubscribers);
      LEDMatrixGFX::backgroundLayer.setFont(gohufont11b);
      LEDMatrixGFX::backgroundLayer.drawString(x-1, y,   rgb24(0,0,0),          szBuffer);
      LEDMatrixGFX::backgroundLayer.drawString(x+1, y,   rgb24(0,0,0),          szBuffer);
      LEDMatrixGFX::backgroundLayer.drawString(x,   y-1, rgb24(0,0,0),          szBuffer);
      LEDMatrixGFX::backgroundLayer.drawString(x,   y+1, rgb24(0,0,0),          szBuffer);
      LEDMatrixGFX::backgroundLayer.drawString(x,   y,   rgb24(255,255,255),    szBuffer);
  }
};

#endif
