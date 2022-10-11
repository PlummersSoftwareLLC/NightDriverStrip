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
// History:     Jul-27-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------


/*
*
* Aurora: https://github.com/pixelmatix/aurora
* Copyright (c) 2014 Jason Coon
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PatternQR_H
#define PatternQR_H

#include "globals.h"
#include "Geometry.h"
#include "qrcode.h"


class PatternQR : public LEDStripEffect 
{
protected:

    String lastData;
    QRCode qrcode;
    uint8_t * qrcodeData = nullptr;
    const int qrVersion = 2;

public:

    PatternQR() : LEDStripEffect("QR")
    {
        qrcodeData = (uint8_t *) PreferPSRAMAlloc(qrcode_getBufferSize(qrVersion));
    }

    virtual ~PatternQR()
    {
        free(qrcodeData);
    }

    virtual void Start()
    {
        graphics()->fillScreen(graphics()->to16bit(CRGB::DarkBlue));
    }

    virtual void Draw()
    {
        String sIP = WiFi.isConnected() ? "http://" + WiFi.localIP().toString() : "No Wifi";
        if (sIP != lastData)
        {
            lastData = sIP;
            qrcode_initText(&qrcode, qrcodeData, qrVersion, ECC_LOW, sIP.c_str());  

            const int leftMargin = MATRIX_CENTER_X - qrcode.size / 2;
            const int topMargin = 4;
            const int borderSize = 2;
            const uint16_t foregroundColor = WHITE16;
            const uint16_t backgroundColor = graphics()->to16bit(CRGB(0,0,144));
            const uint16_t borderColor = BLUE16;
            if (qrcode.size + topMargin + borderSize > MATRIX_HEIGHT - 1)
            throw new runtime_error("Matrix can't hold the QR code height");

            int w = qrcode.size + borderSize * 2;
            int h = w;

            graphics()->fillRect(leftMargin - borderSize, topMargin - borderSize, w, h, BLACK16);
            graphics()->drawRect(leftMargin - borderSize, topMargin - borderSize, w, h, borderColor);

            for (uint8_t y = 0; y < qrcode.size; y++) 
            for (uint8_t x = 0; x < qrcode.size; x++) 
                graphics()->setPixel(leftMargin + x, topMargin + y, (qrcode_getModule(&qrcode, x, y) ? foregroundColor : BLACK16));
        }
    }
};

#endif

