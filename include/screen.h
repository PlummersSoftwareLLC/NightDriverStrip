#pragma once

//+--------------------------------------------------------------------------
//
// File:        screen.h
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
//    Generalizes drawing to various different screens.  For example,
//    drawing a line accepts a color in some cases, but not others, and
//    it depends on what display you are compiling for.  This is a bit
//    of an abstraction layer on those various devices.
//
// History:     Dec-10-2022         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_SCREEN

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "gfxbase.h"
#include "itaskservice.h"

class Screen;
std::unique_ptr<Screen> CreateHardwareScreen(int w, int h);


#if defined(TOGGLE_BUTTON_0) || defined(TOGGLE_BUTTON_1)
    #include "Bounce2.h"
#endif

class Screen; // forward declaration for Page interface

// Page
//
// Abstract base for renderable pages on the small status screen.
// Implement Draw to render the page into the provided Screen. Optional
// hooks handle button presses and whether the page should pause effect
// rotation while visible.

class Page
{
  public:
    virtual ~Page() = default;

    // Human-readable name (for diagnostics)
    virtual std::string Name() const { return std::string("Page"); }

    // Render this page. bRedraw indicates a full redraw is requested.
    virtual void Draw(Screen& display, bool bRedraw) = 0;

    // Optional button hook. Default behavior is provided in screen.cpp.
    virtual void OnButtonPress(uint8_t buttonIndex);
};

// Hardware-specific screen implementations are defined in separate headers:
// - screen_m5.h
// - screen_tft.h
// - screen_amoled.h

class Screen : public GFXBase, public ITaskService
{
  public:
    static std::mutex _screenMutex;

    Screen(int w, int h) : GFXBase(w, h)
    {
    }

    // IService::Name. Used by ITaskService for log lines.
    const char* Name() const override { return "Screen"; }

    // Some devices, like the OLED, require that you send the whole buffer at once, but others do not.  The default impl is to do nothing.

    virtual void StartFrame();

    virtual void EndFrame();

    // Display capabilities and theme colors
    // Default: color display with a blue theme and white/yellow accents.
    virtual bool IsMonochrome() const { return false; }
    virtual uint16_t GetTextColor() const { return WHITE16; }
    virtual uint16_t GetBkgndColor() const { return BLUE16; }
    virtual uint16_t GetBorderColor() const { return YELLOW16; }

    // Define the drawable area for the spectrum to render into the status area

    const int BottomMargin = 12;

    virtual void ScreenStatus(const String &strStatus);

    // fontHeight
    //
    // Returns the height of the current font

    virtual int fontHeight();

    // textHeight
    //
    // Returns the height of a string in screen pixels

    virtual int textHeight(const String & str);

    // textWidth
    //
    // Returns the width of a string in screen pixels

    virtual int textWidth(const String & str);

    // Render the current page into this screen.
    void Update(bool bRedraw);

    // Flip to the next page and handle effect-rotation pause/resume semantics.
    // Safe to call from button handlers.
    static void FlipToNextPage();

  protected:
    // ITaskService hooks. Run() is the screen update loop (button handling
    // + periodic redraw); GetTaskConfig() supplies the FreeRTOS task config.
    TaskConfig GetTaskConfig() const override;
    void Run() override;

  private:
    // Cached screen refresh FPS (updated each loop iteration)
    float _screenFPS = 0.0f;
    uint32_t _lastScreenMillis = 0;
    bool _fpsTouchedThisFrame = false;

  public:

    inline float GetScreenFPS() const { return _screenFPS; }

    inline void SetScreenFPS(float fps)
    {
        _screenFPS = fps;
        _fpsTouchedThisFrame = true;
    }

    // TouchFPS
    // Marks the FPS value as intentionally managed this frame without changing it.
    // This prevents the fallback loop-based FPS from overwriting an effect-driven FPS
    // when the active page prefers to keep the last effect cadence.
    inline void TouchFPS()
    {
        _fpsTouchedThisFrame = true;
    }

    inline void UpdateScreenFPSFromDelta(uint32_t dtMs)
    {
        if (dtMs == 0)
            return;
        const float inst = 1000.0f / (float)dtMs;
        // Light smoothing to avoid flicker
        _screenFPS = (_screenFPS * 0.8f) + (inst * 0.2f);
        _fpsTouchedThisFrame = true;
    }

  private:

  // Page registry and page count helpers
    static std::vector<std::unique_ptr<class Page>>& Pages();
    static int ActivePageCount();

    #if defined(TOGGLE_BUTTON_0)
        Bounce2::Button _button0;
    #endif
    #if defined(TOGGLE_BUTTON_1)
        Bounce2::Button _button1;
    #endif
};

#endif // USE_SCREEN
