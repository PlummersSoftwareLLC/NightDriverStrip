//+--------------------------------------------------------------------------
//
// File:        screen_impl.cpp
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//    Implementations of hardware-specific Screen subclasses.
//    This file contains all the heavy library includes to keep them out of headers.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_SCREEN

#include <memory>
#include "screen.h"

// --- OLEDScreen (Legacy) ---
//
// Display code for the blue OLED display on the Heltect Wifi Kit 32 Original
#if USE_OLED && !USE_SSD1306
    #include <Adafruit_GFX.h>
    #include <gfxfont.h>
    #include <U8g2lib.h>

    class OLEDScreen final : public Screen
    {
        U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled;
    public:
        #if ARDUINO_HELTEC_WIFI_LORA_32_V3
            OLEDScreen(int w, int h) : Screen(w, h), oled(U8G2_R2, /*reset*/ 21, /*clk*/ 18, /*data*/ 17)
        #else
            OLEDScreen(int w, int h) : Screen(w, h), oled(U8G2_R2, /*reset*/ 16, /*clk*/ 15, /*data*/ 4)
        #endif
        {
            oled.begin();
            oled.clear();
        }
        void StartFrame() override { oled.clearBuffer(); }
        void EndFrame() override { oled.sendBuffer(); }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override {
            oled.setDrawColor(color == BLACK16 ? 0 : 1);
            oled.drawPixel(x, y);
        }
        void fillScreen(uint16_t color) override { oled.clearDisplay(); }
        bool IsMonochrome() const override { return true; }
        uint16_t GetTextColor() const override { return WHITE16; }
        uint16_t GetBkgndColor() const override { return BLACK16; }
        uint16_t GetBorderColor() const override { return WHITE16; }
    };
#endif

// --- SSD1306Screen (Legacy) ---
//
// Display code for the SSD1306 display on supported Heltec ESP32 boards
#if USE_SSD1306
    #include <Adafruit_GFX.h>
    #include <gfxfont.h>
    #include <heltec.h>
    #include <U8g2lib.h>

    class SSD1306Screen final : public Screen
    {
    public:
        SSD1306Screen(int w, int h) : Screen(w, h) {
            Heltec.begin(true, false, false);
            #if ROTATE_SCREEN
                Heltec.display->screenRotate(ANGLE_180_DEGREE);
            #endif
        }
        void StartFrame() override {}
        void EndFrame() override { Heltec.display->display(); }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override {
            if (color == BLACK16) Heltec.display->clearPixel(x,y);
            else Heltec.display->setPixel(x,y);
        }
        void fillScreen(uint16_t color) override { Heltec.display->clear(); }
        bool IsMonochrome() const override { return true; }
        uint16_t GetTextColor() const override { return WHITE16; }
        uint16_t GetBkgndColor() const override { return BLACK16; }
        uint16_t GetBorderColor() const override { return WHITE16; }
    };
#endif

// --- ElecrowScreen (Legacy) ---
//
// Display code for the Elecrow display on their 3.5"
#if ELECROW
    #define LGFX_USE_V1
    #include <Adafruit_GFX.h>
    #include <gfxfont.h>
    #include <LovyanGFX.hpp>
    #include <U8g2lib.h>

    #define LCD_MOSI 13
    #define LCD_MISO 14
    #define LCD_SCK  12
    #define LCD_CS    3
    #define LCD_RST  -1
    #define LCD_DC   42

    class ElecrowScreen final : public Screen, lgfx::LGFX_Device
    {
        lgfx::Panel_ILI9488 _panel_instance;
        lgfx::Bus_SPI _bus_instance;
    public:
        ElecrowScreen(int w, int h) : Screen(w, h) {
            {
                auto cfg = _bus_instance.config();
                cfg.spi_host = SPI3_HOST;
                cfg.spi_mode = 0;
                cfg.freq_write = 60000000;
                cfg.freq_read = 16000000;
                cfg.spi_3wire = true;
                cfg.use_lock = true;
                cfg.dma_channel = SPI_DMA_CH_AUTO;
                cfg.pin_sclk = LCD_SCK;
                cfg.pin_mosi = LCD_MOSI;
                cfg.pin_miso = LCD_MISO;
                cfg.pin_dc = LCD_DC;
                _bus_instance.config(cfg);
                _panel_instance.setBus(&_bus_instance);
            }
            {
                auto cfg = _panel_instance.config();
                cfg.pin_cs = LCD_CS;
                cfg.pin_rst = LCD_RST;
                cfg.pin_busy = -1;
                cfg.memory_width = h;
                cfg.memory_height = w;
                cfg.panel_width = h;
                cfg.panel_height = w;
                cfg.offset_x = 0;
                cfg.offset_y = 0;
                cfg.offset_rotation = 0;
                cfg.dummy_read_pixel = 8;
                cfg.dummy_read_bits = 1;
                cfg.readable = true;
                cfg.invert = false;
                cfg.rgb_order = false;
                cfg.dlen_16bit = false;
                cfg.bus_shared = true;
                _panel_instance.config(cfg);
            }
            setPanel(&_panel_instance);
            constexpr auto LCD_BL = 46;
            lgfx::LGFX_Device::init();
            lgfx::LGFX_Device::setRotation(1);
            pinMode(LCD_BL, OUTPUT);
            digitalWrite(LCD_BL, HIGH);
            lgfx::LGFX_Device::fillScreen(TFT_BLUE);
            lgfx::LGFX_Device::setTextDatum(lgfx::baseline_left);
        }
        void startWrite() override { lgfx::LGFX_Device::startWrite(); }
        void endWrite() override { lgfx::LGFX_Device::endWrite(); }
        void StartFrame() override { lgfx::LGFX_Device::startWrite(); }
        void EndFrame() override { lgfx::LGFX_Device::endWrite(); }
        void writePixel(int16_t x, int16_t y, uint16_t color) override { lgfx::LGFX_Device::writePixel(x, y, color); }
        void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { lgfx::LGFX_Device::writeFillRect(x, y, w, h, color); }
        void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override { lgfx::LGFX_Device::writeFastVLine(x, y, h, color); }
        void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override { lgfx::LGFX_Device::writeFastHLine(x, y, w, color); }
        void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override { lgfx::LGFX_Device::drawLine(x0, y0, x1, y1, color); }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override { lgfx::LGFX_Device::drawPixel(x, y, color); }
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { lgfx::LGFX_Device::fillRect(x, y, w, h, color); }
        void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { lgfx::LGFX_Device::drawRect(x, y, w, h, color); }
        void fillScreen(uint16_t color) override { lgfx::LGFX_Device::fillScreen(color); }
    };
#endif

// --- LCDScreen (Legacy / WROVER) ---
//
// Screen class that works with the WROVER devkit board
#if USE_LCD
    #include <Adafruit_ILI9341.h>
    class LCDScreen final : public Screen
    {
        SPIClass hspi;
        std::unique_ptr<Adafruit_ILI9341> pLCD;
    public:
        LCDScreen(int w, int h) : Screen(w, h), hspi(HSPI) {
            hspi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, -1);
            #ifdef TFT_BL
                pinMode(TFT_BL, OUTPUT);
            #endif
            pLCD = std::make_unique<Adafruit_ILI9341>(&hspi, TFT_DC, TFT_CS, TFT_RST);
            pLCD->begin();
            pLCD->setRotation(1);
            pLCD->fillScreen(GREEN16);
        }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override { pLCD->writePixel(x, y, color); }
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { pLCD->fillRect(x, y, w, h, color); }
    };
#endif

// --- M5Screen ---
//
// Screen class that supports the M5 devices
#if USE_M5DISPLAY
    #include <M5Unified.h>
    #include <M5UnitLCD.h>
    class M5Screen final : public Screen
    {
    public:
        M5Screen(int w, int h) : Screen(w, h) { M5.Lcd.fillScreen(GREEN16); }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override { M5.Lcd.drawPixel(x, y, color); }
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { M5.Lcd.fillRect(x, y, w, h, color); }
        void fillScreen(uint16_t color) override { M5.Lcd.fillScreen(color); }
    };
#endif

// --- TFTScreen ---
//
// Screen class that works with the TFT_eSPI library for devices such as the S3-TFT-Feather
#if USE_TFTSPI
    #include <SPI.h>
    #include <TFT_eSPI.h>
    class TFTScreen final : public Screen
    {
        TFT_eSPI tft;
    public:
        TFTScreen(int w, int h) : Screen(w, h) {
            tft.begin();
            #ifdef TFT_BL
                pinMode(TFT_BL, OUTPUT);
                digitalWrite(TFT_BL, 128);
            #endif
            tft.setRotation(3);
            tft.fillScreen(TFT_GREEN);
            tft.setTextDatum(L_BASELINE);
        }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override { tft.drawPixel(x, y, color); }
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { tft.fillRect(x, y, w, h, color); }
        void fillScreen(uint16_t color) override { tft.fillScreen(color); }
    };
#endif

// --- AMOLEDScreen ---
//
// Screen class that works with the AMOLED S3
#if AMOLED_S3
    #include "amoled/LilyGo_AMOLED.h"
    #include "amoled/LV_Helper.h"
    #include <lvgl.h>

    class AMOLEDScreen final : public Screen
    {
        LilyGo_Class amoled;
        lv_color_t * cbuf = NULL;
        lv_obj_t * canvas = NULL;

        inline lv_color_t lv_from16Bit(uint16_t color)
        {
            uint8_t r = gamma5[color >> 11];
            uint8_t g = gamma6[(color >> 5) & 0x3F];
            uint8_t b = gamma5[color & 0x1F];
            return lv_color_make(r, g, b);
        }

    public:
        AMOLEDScreen(int w, int h) : Screen(w, h) {
            if (!amoled.begin()) {
                debugE("AMOLED begin failed");
                return;
            }
            amoled.setBrightness(255);
            beginLvglHelper(amoled);
            const size_t kBufferSize = LV_CANVAS_BUF_SIZE_TRUE_COLOR(w, h);
            cbuf = (lv_color_t *)heap_caps_malloc(kBufferSize, MALLOC_CAP_SPIRAM);
            if (!cbuf) {
                debugE("AMOLED malloc failed");
                return;
            }
            debugW("Allocated %d bytes for lvgl canvas: %p\n", kBufferSize, cbuf);

            canvas = lv_canvas_create(lv_scr_act());
            lv_canvas_set_buffer(canvas, cbuf, w, h, LV_IMG_CF_TRUE_COLOR);
            lv_obj_center(canvas);
            lv_canvas_fill_bg(canvas, lv_from16Bit(GREEN16), LV_OPA_COVER);
        }
        ~AMOLEDScreen() {
            if (canvas) lv_obj_del(canvas);
            if (cbuf) heap_caps_free(cbuf);
        }
        void drawPixel(int16_t x, int16_t y, uint16_t color) override {
            assert(canvas != NULL);
            assert(cbuf != NULL);
            if (canvas) lv_canvas_set_px_color(canvas, x, y, lv_from16Bit(color));
        }
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
            assert(canvas != NULL);
            assert(cbuf != NULL);
            if (!canvas) return;
            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_opa = LV_OPA_COVER;
            rect_dsc.bg_color = lv_from16Bit(color);
            lv_canvas_draw_rect(canvas, x, y, w, h, &rect_dsc);
        }
        void fillScreen(uint16_t color) override {
            assert(canvas);
            assert(cbuf);
            if (canvas) lv_canvas_fill_bg(canvas, lv_from16Bit(color), LV_OPA_COVER);
        }

        // AMOLED is a full color panel but we want a different default theme
        virtual bool IsMonochrome() const override { return false; }
        virtual uint16_t GetTextColor() const override { return Screen::to16bit(CRGB(100, 255, 20)); }
        virtual uint16_t GetBkgndColor() const override { return Screen::to16bit(CRGB::Black); }
        virtual uint16_t GetBorderColor() const override { return Screen::to16bit(CRGB::Red); }
    };
#endif

std::unique_ptr<Screen> CreateHardwareScreen(int w, int h)
{
    #if USE_M5DISPLAY
        return make_unique_psram<M5Screen>(w, h);
    #elif USE_TFTSPI
        return make_unique_psram<TFTScreen>(w, h);
    #elif USE_LCD
        return make_unique_psram<LCDScreen>(w, h);
    #elif ELECROW
        return make_unique_psram<ElecrowScreen>(w, h);
    #elif USE_SSD1306
        return make_unique_psram<SSD1306Screen>(w, h);
    #elif USE_OLED
        return make_unique_psram<OLEDScreen>(w, h);
    #elif AMOLED_S3
        return make_unique_psram<AMOLEDScreen>(w, h);
    #else
        return nullptr;
    #endif
}

#endif // USE_SCREEN
