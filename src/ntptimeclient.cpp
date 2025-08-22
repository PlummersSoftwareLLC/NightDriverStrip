#include "globals.h"
#include "systemcontainer.h"
#include <array>    // std::array
#include <cmath>    // std::abs
#include <cstdint>  // Fixed-width integers
#include <mutex>    // std::lock_guard (assuming already included elsewhere)

// Assuming NTP_PACKET_LENGTH is defined in globals.h as 48
static_assert(NTP_PACKET_LENGTH == 48, "NTP packet length must be 48 bytes");

bool NTPTimeClient::UpdateClockFromWeb(WiFiUDP* pUDP)
{
    // Avoid running NTP during firmware updates as it can disturb the process.
    if (g_Values.UpdateStarted)
    {
        debugW("Update in progress; skipping NTP time check.");
        return false;
    }

    debugV("Updating Clock From Web...");

    // Constants for NTP protocol
    constexpr uint8_t NTP_CLIENT_LI_VN_MODE = 0b00011011; // LI=0, VN=3, Mode=3 (client)
    constexpr uint8_t NTP_SERVER_MODE = 4;
    constexpr uint8_t MIN_NTP_VERSION = 3;
    constexpr uint8_t MAX_NTP_VERSION = 4;
    constexpr uint8_t NTP_LEAP_UNSYNCHRONIZED = 3; // Leap indicator for unsynchronized server
    constexpr uint8_t NTP_STRATUM_KOD = 0;         // Kiss-o'-Death stratum
    constexpr uint8_t NTP_STRATUM_UNSYNCHRONIZED = 16;
    constexpr uint16_t NTP_PORT = 123;
    constexpr int MAX_ATTEMPTS = 100;              // ~10 seconds timeout (100ms delay each)
    constexpr double MAX_TIME_DELTA_SEC = 0.25;    // Threshold for clock adjustment
    constexpr uint32_t NTP_UNIX_EPOCH_DELTA =      // NTP (1900-01-01) to Unix (1970-01-01): 70 years + 17 leap days
        (((70UL * 365UL) + 17UL) * 86400UL);

    // Prepare the NTP request packet
    std::array<uint8_t, NTP_PACKET_LENGTH> packet{};
    packet[0] = NTP_CLIENT_LI_VN_MODE;

    // Resolve the NTP server; fall back to a known good IP if DNS fails.
    IPAddress ntpServer;
    if (WiFi.hostByName(g_ptrSystem->DeviceConfig().GetNTPServer().c_str(), ntpServer) != 1)
    {
        if (!ntpServer.fromString("216.239.35.12")) // time.google.com (as of writing)
        {
            debugW("Failed to resolve NTP server and fallback IP parse failed.");
            return false;
        }
    }

    // Helper to drain UDP buffer efficiently
    auto drain_udp = [](WiFiUDP* udp) {
        uint8_t buffer[256];
        while (udp->available() > 0)
        {
            size_t n = std::min<size_t>(udp->available(), sizeof(buffer));
            udp->read(buffer, n);
        }
    };

    // Drain any stale packets
    int packetSize;
    while ((packetSize = pUDP->parsePacket()) > 0)
    {
        drain_udp(pUDP);
    }

    // Send the request
    if (!pUDP->beginPacket(ntpServer, NTP_PORT))
    {
        debugW("beginPacket failed for NTP request");
        return false;
    }
    if (pUDP->write(packet.data(), NTP_PACKET_LENGTH) != NTP_PACKET_LENGTH)
    {
        debugW("Failed to write full NTP request");
        return false;
    }
    if (!pUDP->endPacket())
    {
        debugW("endPacket failed for NTP request");
        return false;
    }

    debugV("NTP: request sent; waiting for response...");

    // Wait for a valid NTP reply
    std::array<uint8_t, NTP_PACKET_LENGTH> reply{};
    bool gotReply = false;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        packetSize = pUDP->parsePacket();
        if (packetSize <= 0)
        {
            delay(100);
            continue;
        }

        // Ensure reply is from NTP port
        if (pUDP->remotePort() != NTP_PORT)
        {
            drain_udp(pUDP);
            continue;
        }

        // Discard undersized packets
        if (packetSize < NTP_PACKET_LENGTH)
        {
            drain_udp(pUDP);
            continue;
        }

        // Read the NTP header
        if (pUDP->read(reply.data(), NTP_PACKET_LENGTH) != NTP_PACKET_LENGTH)
        {
            debugW("NTP: short read; retrying");
            drain_udp(pUDP); // Drain any remaining data
            continue;
        }

        gotReply = true;
        break;
    }

    if (!gotReply)
    {
        debugW("NTP: timeout waiting for reply");
        return false;
    }

    // Validate NTP header
    const uint8_t li_vn_mode = reply[0];
    const uint8_t leap_indicator = li_vn_mode >> 6;
    const uint8_t version = (li_vn_mode >> 3) & 0x07;
    const uint8_t mode = li_vn_mode & 0x07;
    const uint8_t stratum = reply[1];

    if (mode != NTP_SERVER_MODE ||
        version < MIN_NTP_VERSION || version > MAX_NTP_VERSION ||
        leap_indicator == NTP_LEAP_UNSYNCHRONIZED ||
        stratum == NTP_STRATUM_KOD ||
        stratum >= NTP_STRATUM_UNSYNCHRONIZED)
    {
        debugW("NTP: invalid header (LI=%u, version=%u, mode=%u, stratum=%u)",
               leap_indicator, version, mode, stratum);
        return false;
    }

    auto be32_to_uint32 = [](const uint8_t* p) -> uint32_t {
        return (static_cast<uint32_t>(p[0]) << 24) |
               (static_cast<uint32_t>(p[1]) << 16) |
               (static_cast<uint32_t>(p[2]) <<  8) |
               (static_cast<uint32_t>(p[3]) <<  0);
    };

    // Extract transmit timestamp
    const uint32_t ntp_secs = be32_to_uint32(&reply[40]);
    const uint32_t ntp_frac = be32_to_uint32(&reply[44]);
    const uint64_t usec = (static_cast<uint64_t>(ntp_frac) * 1'000'000ULL) >> 32;

    // Convert to Unix timeval
    struct timeval new_timeval{};
    new_timeval.tv_sec = static_cast<time_t>(ntp_secs - NTP_UNIX_EPOCH_DELTA);
    new_timeval.tv_usec = static_cast<suseconds_t>(usec);

    // Get current time
    struct timeval current_timeval{};
    if (gettimeofday(&current_timeval, nullptr) != 0)
    {
        debugW("gettimeofday failed");
        return false;
    }

    // Compute delta
    const double current_sec = current_timeval.tv_sec + (static_cast<double>(current_timeval.tv_usec) / 1'000'000.0);
    const double new_sec = new_timeval.tv_sec + (static_cast<double>(new_timeval.tv_usec) / 1'000'000.0);
    const double delta_sec = std::abs(new_sec - current_sec);

    debugI("NTP: current=%lf new=%lf delta=%.3f sec", current_sec, new_sec, delta_sec);

    // Update clock if necessary, under mutex
    std::lock_guard<std::mutex> guard(_clockMutex);
    bool adjusted = false;
    if (delta_sec >= MAX_TIME_DELTA_SEC)
    {
        debugI("NTP: adjusting by %.3f seconds", delta_sec);
        if (settimeofday(&new_timeval, nullptr) != 0)
        {
            debugW("settimeofday failed");
            return false;
        }
        adjusted = true;
    }
    else
    {
        debugI("NTP: delta %.3f < %.3f sec; not updating RTC", delta_sec, MAX_TIME_DELTA_SEC);
    }
    _bClockSet = true; // Mark successful sync

    // Log friendly timestamp (local time)
    char timestamp_str[128]{};
    struct tm* local_tm = localtime(&new_timeval.tv_sec);
    if (local_tm && strftime(timestamp_str, sizeof(timestamp_str), "%d %b %Y %H:%M:%S", local_tm) > 0)
    {
        if (adjusted)
            debugI("NTP: set time to %s.%06ld (delta=%.3f sec)", timestamp_str, static_cast<long>(new_timeval.tv_usec), delta_sec);
        else
            debugI("NTP: time already close; latest is %s.%06ld (delta=%.3f sec)", timestamp_str, static_cast<long>(new_timeval.tv_usec), delta_sec);
    }

    return true;
}

// Convenience utility for debugging uptime and reset reason.
// Callable from gdb, serial shell, or network log debugger.
void NTPTimeClient::ShowUptime()
{
    // Get uptime in microseconds since boot
    const int64_t uptime_us = esp_timer_get_time();
    struct timeval uptime_tv{};
    uptime_tv.tv_sec = uptime_us / 1'000'000LL;
    uptime_tv.tv_usec = uptime_us % 1'000'000LL;

    // Compute days and HH:MM:SS (ignoring leap seconds)
    constexpr int64_t SEC_PER_DAY = 86'400LL;
    const int ndays = static_cast<int>(uptime_tv.tv_sec / SEC_PER_DAY);
    char time_str[64]{};
    struct tm* gm_tm = gmtime(&uptime_tv.tv_sec);
    if (gm_tm)
    {
        strftime(time_str, sizeof(time_str), "%H:%M:%S", gm_tm);
    }
    else
    {
        snprintf(time_str, sizeof(time_str), "Invalid");
    }
    debugI("Uptime: %d days - %s", ndays, time_str);

    // Reset reason lookup
    static const std::unordered_map<esp_reset_reason_t, std::string> reset_reasons = {
        {ESP_RST_UNKNOWN,       "Unknown"},
        {ESP_RST_POWERON,       "Power On"},
        {ESP_RST_EXT,           "External Pin"},
        {ESP_RST_SW,            "Software Restart"},
        {ESP_RST_PANIC,         "Panic"},
        {ESP_RST_INT_WDT,       "Interrupt Watchdog"},
        {ESP_RST_TASK_WDT,      "Task Watchdog"},
        {ESP_RST_WDT,           "Other Watchdog"},
        {ESP_RST_DEEPSLEEP,     "Deep Sleep Wakeup"},
        {ESP_RST_BROWNOUT,      "Brownout"},
        {ESP_RST_SDIO,          "SDIO"},
    };

    const esp_reset_reason_t reason = esp_reset_reason();
    auto it = reset_reasons.find(reason);
    const std::string reason_text = (it != reset_reasons.end()) ? it->second : "Unknown";
    debugI("Last boot reason: (%d): %s", static_cast<int>(reason), reason_text.c_str());
}