//+--------------------------------------------------------------------------
//
// File:        secrets.h
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
//    secret definitions, at the moment this is just network secrets,
//    but could be expanded in the future
//
//---------------------------------------------------------------------------

// NOTE: do NOT enter your network details in this file (secrets.example.h)!
// Instead, copy this file to secrets.h, and set the below defines in that file!

#define cszSSID              "Your SSID"
#define cszPassword          "Your PASS"
#define cszHostname          ""                     // An empty hostname will make it default to esp32-XXXXXX,
                                                    //   with the Xs being 3 bytes of the device's MAC address
#define cszOpenWeatherAPIKey ""                     // Your OpenWeatherMap API key goes Here
#define cszLocation          "98074"
#define bLocationIsZip       true
#define cszCountryCode       "us"                   // Look up the Alpha-2 code for your country at https://www.iban.com/country-codes
#define cszTimeZone          "America/Los_Angeles"
