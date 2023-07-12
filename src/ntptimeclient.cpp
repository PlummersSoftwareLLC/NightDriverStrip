#include "globals.h"
#include "systemcontainer.h"

bool NTPTimeClient::UpdateClockFromWeb(WiFiUDP * pUDP)
{
    debugV("Updating Clock From Web...");

    std::lock_guard<std::mutex> guard(_clockMutex);

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

    IPAddress ipNtpServer;
    if (WiFi.hostByName(g_ptrSystem->DeviceConfig().GetNTPServer().c_str(), ipNtpServer) != 1)
        ipNtpServer.fromString("216.239.35.12"); // Use Google Time as default. The pool.ntp.org servers (IPs) don't necessarily last very long.

    // Send the ntp packet.
    while (pUDP->parsePacket() != 0)
        pUDP->flush();

    pUDP->beginPacket(ipNtpServer, 123);
    pUDP->write((const uint8_t *)chNtpPacket, NTP_PACKET_LENGTH);
    pUDP->endPacket();

    debugV("NTP clock: ntp packet sent to ntp server.");
    debugV("NTP clock: awaiting response from ntp server");

    int iPass = 0;
    while (!pUDP->parsePacket())
    {
        pUDP->flush();
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

    // BUGBUG (davepl): I've been gettin back odd packets where the clock is year 2036 and micros is 0. I ignore those.

    if (microsecs == 0)
    {
        debugW("Bogus NTP time received, ignoring.");
        return false;
    }

    debugV("NTP clock: Raw values sec=%u, usec=%u", frac, microsecs);

    tvNew.tv_sec = ((unsigned long)chNtpPacket[40] << 24) +       // bits 24 through 31 of ntp time
        ((unsigned long)chNtpPacket[41] << 16) +                        // bits 16 through 23 of ntp time
        ((unsigned long)chNtpPacket[42] << 8) +                         // bits  8 through 15 of ntp time
        ((unsigned long)chNtpPacket[43]) -                              // bits  0 through  7 of ntp time
        (((70UL * 365UL) + 17UL) * 86400UL);                            // ntp to unix conversion

    tvNew.tv_usec = microsecs;

    timeval tvOld;
    gettimeofday(&tvOld, nullptr);

    float dOld = tvOld.tv_sec + (tvOld.tv_usec / (float) MICROS_PER_SECOND);
    float dNew = tvNew.tv_sec + (tvNew.tv_usec / (float) MICROS_PER_SECOND);

    // If the clock is off by more than a quarter second, update it

    auto delta = fabs(dNew - dOld);
    if (delta < 0.25)
    {
        debugV("Clock is only off by %lf so not updating the RTC.", delta);
    }
    else
    {
        debugV("Adjusting time by %lf to %lf", delta, dNew);
        settimeofday(&tvNew, NULL);                                 // Set the ESP32 rtc.
        time_t newtime = time(NULL);
        debugV("New Time: %s", ctime(&newtime));
    }

    // Time has been received.
    // Output date and time to serial.

    char chBuffer[64];
    struct tm * tmPointer = localtime(&tvNew.tv_sec);
    strftime(chBuffer, sizeof(chBuffer), "%d %b %y %H:%M:%S", tmPointer);
    debugV("NTP clock: response received, time written to ESP32 rtc: %ld.%ld, DELTA: %lf\n",
            tvNew.tv_sec,
            tvNew.tv_usec,
            dNew - dOld );

    _bClockSet = true;  // Clock has been set at least once

    return true;
}
