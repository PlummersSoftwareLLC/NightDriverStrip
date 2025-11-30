#pragma once

#include <Arduino.h>
#include <WiFi.h>

// Define a test mode to enable/disable the WiFi test framework
#ifdef ENABLE_WIFI_TEST_MODE

// Forward declarations
void WiFiTestLoopEntry(void* pvParameters);

// --- Test Command Definitions ---
enum class WiFiTestCommand : uint8_t {
    SET_CREDENTIALS,        // Set WiFi credentials (SSID, Password)
    CLEAR_ALL_CREDENTIALS,  // Clear all stored WiFi credentials
    EXPECT_STA_CONNECTION,  // Expect STA connection (SSID)
    EXPECT_AP_MODE,         // Expect device to be in AP mode
    DISABLE_AP_MODE,        // Ensure AP mode is off (e.g., after STA connection)
    START_CAPTIVE_PORTAL,   // Explicitly start Captive Portal
    WAIT_FOR_MS,            // Wait for a specified number of milliseconds
    LOG_MESSAGE,            // Log a custom message to serial
    REBOOT_DEVICE           // Request a device reboot
};

// --- Test Step Structure ---
struct WiFiTestStep {
    WiFiTestCommand command;
    const char* ssid;       // Used by SET_CREDENTIALS, EXPECT_STA_CONNECTION
    const char* password;   // Used by SET_CREDENTIALS
    uint32_t timeoutMs;     // Used by EXPECT_STA_CONNECTION, EXPECT_AP_MODE, WAIT_FOR_MS
    wl_status_t expectedStatus; // Used by EXPECT_STA_CONNECTION (e.g., WL_CONNECTED, WL_CONNECT_FAILED)
    const char* message;    // Used by LOG_MESSAGE

    // Constructor for commands with SSID/Password/Timeout/Status
    WiFiTestStep(WiFiTestCommand cmd, const char* s = nullptr, const char* p = nullptr, uint32_t t = 0, wl_status_t es = WL_IDLE_STATUS, const char* msg = nullptr)
        : command(cmd), ssid(s), password(p), timeoutMs(t), expectedStatus(es), message(msg) {}
};

// --- Test Case Structure ---
struct WiFiTestCase {
    const char* name;
    WiFiTestStep* steps;
    size_t numSteps;

    WiFiTestCase(const char* n, WiFiTestStep* s, size_t ns)
        : name(n), steps(s), numSteps(ns) {}
};

// Global array of test cases (defined in .cpp)
extern WiFiTestCase* g_wifiTestCases[];
extern size_t g_numWiFiTestCases;

#endif // ENABLE_WIFI_TEST_MODE
