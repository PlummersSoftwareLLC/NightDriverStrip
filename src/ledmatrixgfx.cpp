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

#include "gfxbase.h"
#include "globals.h"
#include "effectmanager.h"

extern DRAM_ATTR AppTime g_AppTime;                        // Keeps track of frame times
extern DRAM_ATTR std::shared_ptr<GFXBase> g_pDevices[NUM_CHANNELS];
extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_pEffectManager;

uint32_t GFXBase::noise_x;
uint32_t GFXBase::noise_y;
uint32_t GFXBase::noise_z;
uint32_t GFXBase::noise_scale_x;
uint32_t GFXBase::noise_scale_y;

uint8_t GFXBase::noise[MATRIX_WIDTH][MATRIX_HEIGHT];   // BUGBUG Could this go in PSRAM if allocated instead?
uint8_t GFXBase::noisesmoothing;

#ifdef USEMATRIX

  #include <SmartMatrix.h>
  #include "ledmatrixgfx.h"
    
  SMLayerIndexed<SM_RGB, kIndexedLayerOptions> indexedLayer(kMatrixWidth, kMatrixHeight);  
  SMLayerScrolling<SM_RGB, kScrollingLayerOptions> scrollingLayer(kMatrixWidth, kMatrixHeight);  
  SMLayerBackground<SM_RGB, kBackgroundLayerOptions> backgroundLayer(kMatrixWidth, kMatrixHeight);
  SmartMatrixHub75Refresh<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrixRefresh; 
  SmartMatrixHub75Calc<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix;

  double frameStartTime = 0;
    
void LEDMatrixGFX::StartMatrix()
{
    matrix.addLayer(&backgroundLayer);
    matrix.begin(100000);

    backgroundLayer.fillScreen(rgb24(0, 64, 0));
    backgroundLayer.setFont(font5x7);
    backgroundLayer.drawString(8, kMatrixHeight / 2 - 6, rgb24(255,255,255), "NightDriver");
    backgroundLayer.swapBuffers(false);    

    matrix.setBrightness(255);
}

CRGB * LEDMatrixGFX::GetMatrixBackBuffer()
{
    for (int i = 0; i < NUM_CHANNELS; i++)
      g_pDevices[i]->UpdatePaletteCycle();

    return (CRGB *) backgroundLayer.getRealBackBuffer();

}

void LEDMatrixGFX::MatrixSwapBuffers()
{
  frameStartTime = g_AppTime.CurrentTime();
  backgroundLayer.swapBuffers(true);
}

void LEDMatrixGFX::PresentFrame()
{
    const double minimumFrameTime = 1.0/g_pEffectManager->GetCurrentEffect()->DesiredFramesPerSecond();
    double elapsed = g_AppTime.CurrentTime() - frameStartTime;
    if (elapsed < minimumFrameTime)
        delay((minimumFrameTime-elapsed) * MILLIS_PER_SECOND);

}

#endif
