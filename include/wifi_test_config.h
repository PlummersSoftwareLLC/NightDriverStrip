#pragma once

// --- WiFi Test Configuration ---
//
// This file centralizes all user-configurable parameters for the automated WiFi test framework.
// Developers should modify the values below to match their test environment.

// --- General Test Parameters ---

// SSID for a non-existent network (Harrie Case)
#define TEST_NON_EXISTENT_SSID "NON_EXISTENT_SSID_XYZ" // Ensure this SSID does not exist in your environment

// --- Your Home Network Credentials (for Mistyped and Correct Password Cases) ---
// IMPORTANT: Replace these with your actual home network details.
// For 'Mistyped Password Case', the TEST_HOME_WRONG_PASSWORD should be intentionally incorrect.

#define TEST_HOME_SSID          "Board-1"      // Replace with your actual home WiFi SSID
#define TEST_HOME_CORRECT_PASSWORD "Cherry#1" // Replace with your actual home WiFi password
#define TEST_HOME_WRONG_PASSWORD   "WRONG_PASSWORD"       // An intentionally wrong password for your home WiFi

// --- Dummy Credentials (for clearsettings + startportal robustness test) ---
// These are used for scenarios where some credentials need to be present initially
// but are not critical for actual connection (e.g., to trigger STA mode attempts).

#define TEST_DUMMY_SSID         "TEST_DUMMY_SSID_AP"
#define TEST_DUMMY_PASSWORD     "TEST_DUMMY_PASSWORD"

// --- Test Timeouts ---
// Adjust if your environment or ESP32 model requires longer stabilization times.

#define TEST_SHORT_TIMEOUT_MS   35000   // General timeout for short waits, e.g., Harrie/Mistyped Password
#define TEST_LONG_TIMEOUT_MS    950000  // Timeout for Dave Case (not yet implemented)
#define TEST_BOOT_STABILIZE_MS  5000    // Time to wait after boot for system stabilization
#define TEST_AP_STABILIZE_MS    10000   // Time to wait for AP mode to become active

// --- Test Control Files ---
#define TEST_STATE_FILE         "/test_state.json" // File to store test state across reboots
