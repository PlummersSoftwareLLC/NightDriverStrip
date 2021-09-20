//+--------------------------------------------------------------------------
//
// File:        NTPTimeClient.h
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
//    Sets the system clock from the specified NTP Server
//
// History:     Jul-12-2018         Davepl      Created for BigBlueLCD
//				Oct-09-2018			Davepl		Copied to LEDWifi project
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include <sys/cdefs.h>
#include <sys/time.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <mutex>

// NTPTimeClient
//
// Basically, I took some really ancient NTP code that I had on hand that I knew
// worked and wrapped it in a class.  As expected, it works, but it could likely
// benefit from cleanup or even wholesale replacement.

class NTPTimeClient
{
	static bool _bClockSet;	
	static std::mutex  _mutex;

  public:

	NTPTimeClient()
	{ 
	}

	static inline bool HasClockBeenSet()
	{
		return _bClockSet;
	}

	static inline int GetTimeZone()
	{
		return TIME_ZONE;
	}

	static bool UpdateClockFromWeb(WiFiUDP * pUDP)
	{
		debugI("Updating Clock From Web...");

		std::lock_guard<std::mutex> guard(_mutex);

		char chNtpPacket[NTP_PACKET_LENGTH];
		memset(chNtpPacket, 0, NTP_PACKET_LENGTH);		
		
		// Send ntp time request.
		// Initialize ntp packet.


		// Set the ll (leap indicator), vvv (version number) and mmm (mode) bits.
		//  
		//  These bits are contained in the first byte of chNtpPacker and are in
		// the following format:  llvvvmmm
		//
		// where:
		//
		//    ll  (leap indicator) = 0
		//    vvv (version number) = 3
		//    mmm (mode)           = 3

		chNtpPacket[0] = 0b00011011;

		// Send the ntp packet.

		IPAddress ipNtpServer(192, 168, 1, 2); 								// My Synology NAS
		//IPAddress ipNtpServer(216, 239, 35, 12); 							// 216.239.35.12 Google Time
		//IPAddress ipNtpServer(17, 253, 16, 253);							// Apple time

		pUDP->beginPacket(ipNtpServer, 123);
		pUDP->write((const uint8_t *)chNtpPacket, NTP_PACKET_LENGTH);
		pUDP->endPacket();

		debugV("NTP clock: ntp packet sent to ntp server.");
		debugV("NTP clock: awaiting response from ntp server");

		int iPass = 0;
		while (!pUDP->parsePacket())
		{
			delay(100);
			debugV(".");
			if (iPass++ > 100)
			{
				debugW("NTP clock: TIMEOUT after 10 seconds of parsePacket!");
				return false;
			}
		}
		debugV("NTP clock: Time Received From Server...");

		// Server responded, read the packet.

		if (NTP_PACKET_LENGTH != pUDP->read(chNtpPacket, NTP_PACKET_LENGTH))
		{
			debugE("Unexpected number of bytes back from UDP read, so returning early");
			return false;
		}

		// Obtain the time from the packet, convert to Unix time, and adjust for the time zone.

		struct timeval tvNew = { 0 };

		uint32_t frac  = (uint32_t) chNtpPacket[44] << 24
						| (uint32_t) chNtpPacket[45] << 16
						| (uint32_t) chNtpPacket[46] <<  8
						| (uint32_t) chNtpPacket[47] <<  0;

		uint32_t microsecs = ((uint64_t) frac * 1000000) >> 32;

		debugV("NTP clock: Raw values sec=%u, usec=%u", frac, microsecs);
		
		tvNew.tv_sec = ((unsigned long)chNtpPacket[40] << 24) +       // bits 24 through 31 of ntp time
			((unsigned long)chNtpPacket[41] << 16) +       					// bits 16 through 23 of ntp time
			((unsigned long)chNtpPacket[42] << 8) +       					// bits  8 through 15 of ntp time
			((unsigned long)chNtpPacket[43]) -             					// bits  0 through  7 of ntp time
			(((70UL * 365UL) + 17UL) * 86400UL);          					// ntp to unix conversion

		tvNew.tv_usec = microsecs;

		timeval tvOld;
		gettimeofday(&tvOld, nullptr);

		double dOld = tvOld.tv_sec + (tvOld.tv_usec / (double) MICROS_PER_SECOND);
		double dNew = tvNew.tv_sec + (tvNew.tv_usec / (double) MICROS_PER_SECOND);
		settimeofday(&tvNew, NULL);									// Set the ESP32 rtc.

		// Time has been received.
		// Output date and time to serial.

		char chBuffer[64];
		struct tm * tmPointer = localtime(&tvNew.tv_sec);
		strftime(chBuffer, sizeof(chBuffer), "%d %b %y %H:%M:%S", tmPointer);
		debugI("NTP clock: response received, time written to ESP32 rtc: %ld.%ld, DELTA: %lf\n", 
				tvNew.tv_sec, 
				tvNew.tv_usec, 
				dNew - dOld );
		
		_bClockSet = true;	// Clock has been set at least once
		
		return true;
	}
};




