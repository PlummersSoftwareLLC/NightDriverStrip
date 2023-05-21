//+--------------------------------------------------------------------------
//
// File:        ledmatrixgfx.cpp
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
// Description:
//
//    Code for handling HUB75 matrix panels
//
// History:     May-24-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

extern DRAM_ATTR AppTime g_AppTime; // Keeps track of frame times
extern DRAM_ATTR std::shared_ptr<GFXBase> g_aptrDevices[NUM_CHANNELS];
extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager;

#if USE_MATRIX

#include <SmartMatrix.h>
#include "ledmatrixgfx.h"

const rgb24 LEDMatrixGFX::defaultBackgroundColor = {0x40, 0, 0};

// The delcarations create the "layers" that make up the matrix display

SMLayerBackground<LEDMatrixGFX::SM_RGB, LEDMatrixGFX::kBackgroundLayerOptions> LEDMatrixGFX::backgroundLayer(LEDMatrixGFX::kMatrixWidth, LEDMatrixGFX::kMatrixHeight);
SMLayerBackground<LEDMatrixGFX::SM_RGB, LEDMatrixGFX::kBackgroundLayerOptions> LEDMatrixGFX::titleLayer(LEDMatrixGFX::kMatrixWidth, LEDMatrixGFX::kMatrixHeight);
SmartMatrixHub75Refresh<COLOR_DEPTH, LEDMatrixGFX::kMatrixWidth, LEDMatrixGFX::kMatrixHeight, LEDMatrixGFX::kPanelType, LEDMatrixGFX::kMatrixOptions> matrixRefresh;

void LEDMatrixGFX::StartMatrix()
{
  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&titleLayer);

  // When the matrix starts, you can ask it to leave N bytes of memory free, and this amount must be tuned.  Too much free 
  // will cause a dim panel with a low refresh, too little will starve other things.  We currently have enough RAM for
  // use so begin() is not being called with a reserve paramter, but it can be if memory becomes scarce.
  
  matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
  matrix.setRefreshRate(MATRIX_REFRESH_RATE);
  matrix.begin();

  Serial.printf("Matrix Refresh Rate: %d\n", matrix.getRefreshRate());

  //backgroundLayer.setRefreshRate(100);
  backgroundLayer.fillScreen(rgb24(0, 64, 0));
  backgroundLayer.setFont(font6x10);
  backgroundLayer.drawString(8, kMatrixHeight / 2 - 6, rgb24(255, 255, 255), "NightDriver");
  backgroundLayer.swapBuffers(false);

  matrix.setBrightness(255);
}

CRGB *LEDMatrixGFX::GetMatrixBackBuffer()
{
  for (int i = 0; i < NUM_CHANNELS; i++)
    g_aptrDevices[i]->UpdatePaletteCycle();

  return (CRGB *)backgroundLayer.getRealBackBuffer();
}

void LEDMatrixGFX::MatrixSwapBuffers(bool bSwapBackground, bool bSwapTitle)
{
  // If an effect redraws itself entirely ever frame, it can skip saving the most recent buffer, so
  // can swap without waiting for a copy.
  matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
  matrix.setRefreshRate(MATRIX_REFRESH_RATE);
  matrix.setMaxCalculationCpuPercentage(100);

  backgroundLayer.swapBuffers(bSwapBackground);
  titleLayer.swapBuffers(bSwapTitle);
}

#endif
