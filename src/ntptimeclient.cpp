#if ENABLE_NTP

#include "globals.h"
#include "systemcontainer.h"

// Static member definitions
DRAM_ATTR bool NTPTimeClient::_bClockSet = false;
DRAM_ATTR std::mutex NTPTimeClient::_clockMutex;

bool NTPTimeClient::UpdateClockFromWeb(WiFiUDP * pUDP)
{
    if (g_Values.UpdateStarted)
    {
        debugW("Update already in progress, skipping time check, as it seems to disturb the update process.");
        return false;
    }

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

    if (!pUDP->beginPacket(ipNtpServer, 123))
    {
        debugW("beginPacket failed in time set");
        return false;
    }

    if (NTP_PACKET_LENGTH != pUDP->write((const uint8_t *)chNtpPacket, NTP_PACKET_LENGTH))
    {
        debugW("Could not write to UDP in time set");
        return false;
    }

    if (!pUDP->endPacket())
    {
        debugW("EndPacket failed in time set.");
        return false;
    }

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

    struct timeval tvNew;
    memset(&tvNew, 0, sizeof(tvNew));

    auto frac  =  (uint32_t) chNtpPacket[44] << 24
                | (uint32_t) chNtpPacket[45] << 16
                | (uint32_t) chNtpPacket[46] <<  8
                | (uint32_t) chNtpPacket[47] <<  0;
    auto microsecs = ((uint64_t) frac * 1000000) >> 32;

    // BUGBUG (davepl): I've been getting back odd packets where the clock is year 2036 and micros is 0. I ignore those.

    if (microsecs == 0)
    {
        debugW("Bogus NTP time received, ignoring.");
        return false;
    }

    debugW("NTP clock: Raw values sec=%lu, usec=%llu", (unsigned long)frac, microsecs);

    tvNew.tv_sec = ((unsigned long)chNtpPacket[40] << 24) +       // bits 24 through 31 of ntp time
        ((unsigned long)chNtpPacket[41] << 16) +                        // bits 16 through 23 of ntp time
        ((unsigned long)chNtpPacket[42] << 8) +                         // bits  8 through 15 of ntp time
        ((unsigned long)chNtpPacket[43]) -                              // bits  0 through  7 of ntp time
        (((70UL * 365UL) + 17UL) * 86400UL);                            // ntp to unix conversion
    tvNew.tv_usec = microsecs;

    timeval tvOld;
    gettimeofday(&tvOld, nullptr);

    double dOld = tvOld.tv_sec + ((double) tvOld.tv_usec / MICROS_PER_SECOND);
    double dNew = tvNew.tv_sec + ((double) tvNew.tv_usec / MICROS_PER_SECOND);

    debugI("Old Time: %lf, New Time: %lf, Delta: %lf", dOld, dNew, abs(dNew - dOld));

    // If the clock is off by more than a quarter second, update it

    constexpr auto kMaxTimeDelta = 0.25;

    auto delta = abs(dNew - dOld);
    if (delta < kMaxTimeDelta)
    {
        debugI("Clock is only off by %lf so not updating the RTC.", delta);
    }
    else
    {
        debugI("Adjusting time by %lf to %lf", delta, dNew);
        settimeofday(&tvNew, nullptr);                                 // Set the ESP32 rtc.
    }

    // Time has been received.
    // Output date and time to serial.

    char chBuffer[128];
    struct tm * tmPointer = localtime(&tvNew.tv_sec);
    strftime(chBuffer, sizeof(chBuffer), "%d %b %Y %H:%M:%S", tmPointer);
    debugI("NTP clock: response received, updated time to: %lld.%lld, DELTA: %lf\n",
            (long long)tvNew.tv_sec,
            (long long)tvNew.tv_usec,
            dNew - dOld );

    _bClockSet = true;  // Clock has been set at least once

    return true;
}

#endif
