//+--------------------------------------------------------------------------
//
// File:        PatternQR.h
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
//   Displays a QR code that links to the unit's IP
//
// History:     Jul-27-2022         Davepl      Created
//
//---------------------------------------------------------------------------

#ifndef PatternQR_H
#define PatternQR_H

#include "qrcode.h"

class PatternQR : public LEDStripEffect
{
    void construct()
    {
        qrcodeData = (uint8_t *) PreferPSRAMAlloc(qrcode_getBufferSize(qrVersion));
        lastData = "";
    }

protected:

    String lastData;
    QRCode qrcode;
    uint8_t * qrcodeData = nullptr;
    const int qrVersion = 2;

public:

    PatternQR() : LEDStripEffect(EFFECT_MATRIX_QR, "QR")
    {
        construct();
    }

    PatternQR(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
        construct();
    }

    virtual ~PatternQR()
    {
        free(qrcodeData);
    }

    virtual void Start() override
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 20;
    }

    virtual void Draw() override
    {
        String sIP = WiFi.isConnected() ? "http://" + WiFi.localIP().toString() : "No Wifi";
        if (sIP != lastData)
        {
            lastData = sIP;
            qrcode_initText(&qrcode, qrcodeData, qrVersion, ECC_LOW, sIP.c_str());
        }
        g()->fillScreen(g()->to16bit(CRGB::DarkBlue));
        const int leftMargin = MATRIX_CENTER_X - qrcode.size / 2;
        const int topMargin = 4;
        const int borderSize = 2;
        const uint16_t foregroundColor = WHITE16;
        const uint16_t backgroundColor = g()->to16bit(CRGB(0,0,144));
        const uint16_t borderColor = BLUE16;
        if (qrcode.size + topMargin + borderSize > MATRIX_HEIGHT - 1)
        throw std::runtime_error("Matrix can't hold the QR code height");

        int w = qrcode.size + borderSize * 2;
        int h = w;

        g()->fillRect(leftMargin - borderSize, topMargin - borderSize, w, h, BLACK16);
        g()->drawRect(leftMargin - borderSize, topMargin - borderSize, w, h, borderColor);

        for (uint8_t y = 0; y < qrcode.size; y++)
            for (uint8_t x = 0; x < qrcode.size; x++)
                g()->setPixel(leftMargin + x, topMargin + y, (qrcode_getModule(&qrcode, x, y) ? foregroundColor : BLACK16));
    }
};

#endif

