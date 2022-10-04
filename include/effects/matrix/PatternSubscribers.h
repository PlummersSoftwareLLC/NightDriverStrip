//+--------------------------------------------------------------------------
//
// File:        PatternSubscribers.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//
// Description:
//
//   Effect code ported from Aurora to Mesmerizer's draw routines
//
// History:     Jun-25-202         Davepl      Adapted from own code
//
//---------------------------------------------------------------------------

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
  
  PatternSubscribers() : LEDStripEffect("Subs")
  {
  }

  static volatile long cSubscribers;
  static volatile long cViews;
  static const char szChannelID[];
  static const char szChannelName1[];

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
