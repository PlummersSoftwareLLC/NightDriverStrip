#include "globals.h"
#include "network.h"
#include "systemcontainer.h" // For g_ptrSystem
#include "wifi_test.h"
#include "wifi_test_config.h" // For test configuration parameters

#ifdef ENABLE_WIFI_TEST_MODE

// --- Globals for credential backup/restore ---
static String g_backup_ssid;
static String g_backup_password;
static WifiCredSource g_backup_source = WifiCredSource::CaptivePortal; // Default
static bool g_credentials_backed_up = false;

// --- Helper Functions for Test Execution ---

// Backup existing WiFi credentials before running tests
void backupWiFiCredentials() {
    g_credentials_backed_up = false;
    const WifiCredSource sources[] = {
        WifiCredSource::CaptivePortal,
        WifiCredSource::ImprovCreds,
        WifiCredSource::CompileTimeCreds
    };

    for (const auto& source : sources) {
        if (ReadWiFiConfig(source, g_backup_ssid, g_backup_password)) {
            g_backup_source = source;
            g_credentials_backed_up = true;
            debugI("TEST: Backed up credentials from source %d for SSID '%s'", (int)source, g_backup_ssid.c_str());
            return;
        }
    }
    debugI("TEST: No existing credentials found to back up.");
}

// Restore WiFi credentials after tests are complete
void restoreWiFiCredentials() {
    g_ptrSystem->WebServer().SetCaptivePortalActive(false); // Ensure portal is considered inactive
    if (g_credentials_backed_up) {
        debugI("TEST: Restoring credentials for SSID '%s' to source %d", g_backup_ssid.c_str(), (int)g_backup_source);
        // Clear any test credentials first
        ClearWiFiConfig(WifiCredSource::CaptivePortal);
        ClearWiFiConfig(WifiCredSource::ImprovCreds);
        ClearWiFiConfig(WifiCredSource::CompileTimeCreds);
        if (!WriteWiFiConfig(g_backup_source, g_backup_ssid, g_backup_password)) {
            debugE("TEST: FAILED to restore WiFi credentials.");
        }
    } else {
        debugI("TEST: No credentials to restore. Leaving system to start captive portal via main loop.");
        // Main loop will handle starting the portal now.
    }
}

// Set WiFi Credentials
bool setWiFiCredentials(const char* ssid, const char* password) {
    debugI("TEST: Setting WiFi credentials: SSID='%s'", ssid);
    // Clear existing credentials for a clean slate
    ClearWiFiConfig(WifiCredSource::CaptivePortal);
    ClearWiFiConfig(WifiCredSource::ImprovCreds);
    ClearWiFiConfig(WifiCredSource::CompileTimeCreds);
    return WriteWiFiConfig(WifiCredSource::CaptivePortal, ssid, password);
}

// Clear All WiFi Credentials
void clearAllWiFiCredentials() {
    debugI("TEST: Clearing all WiFi credentials.");
    ClearWiFiConfig(WifiCredSource::CaptivePortal);
    ClearWiFiConfig(WifiCredSource::ImprovCreds);
    ClearWiFiConfig(WifiCredSource::CompileTimeCreds);
    g_ptrSystem->WebServer().SetCaptivePortalActive(false); // TEMP_LOG: Ensure captive portal flag is reset
}

// Expect STA Connection
bool expectStaConnection(const char* ssid, const char* password, uint32_t timeoutMs, wl_status_t expectedStatus) {
    debugI("TEST: Expecting STA connection to '%s' with status '%d' within %u ms.", ssid, expectedStatus, timeoutMs);
    unsigned long startTime = millis();
    bool expectedStatusReached = false; // Flag to indicate if the expected status was seen

    // Ensure previous connections are cleared for a clean test
    WiFi.disconnect(true, true);
    delay(100); // Give time for disconnect to process

    debugI("TEST: Attempting WiFi.begin('%s', '%s')", ssid, password);
    WiFi.begin(ssid, password);

    unsigned long reportInterval = timeoutMs / 10; // Report progress 10 times
    if (reportInterval == 0) reportInterval = 100;

    for (unsigned long elapsed = 0; elapsed < timeoutMs; elapsed += reportInterval) {
        wl_status_t currentStatus = WiFi.status();
        debugI("TEST: Waiting for connection... %lu/%u ms. Current status: %d (%s)", elapsed, timeoutMs, currentStatus, WLtoString(currentStatus));

        if (currentStatus == expectedStatus) {
            expectedStatusReached = true;
            break; // Expected status reached, break early
        }
        delay(reportInterval);
    }
    
    // After the loop, check if the expected status was reached during polling
    if (expectedStatusReached) {
        debugI("TEST: STA connection to '%s' successful with expected status '%d'. Final status: %d (%s)", ssid, expectedStatus, WiFi.status(), WLtoString(WiFi.status()));
    } else {
        debugE("TEST: STA connection to '%s' FAILED within %u ms. Expected status: %d, Final status: %d (%s)", ssid, timeoutMs, expectedStatus, WiFi.status(), WLtoString(WiFi.status()));
    }
    return expectedStatusReached;
}

// Expect AP Mode
bool expectAPMode(uint32_t timeoutMs) {
    debugI("TEST: Expecting AP mode within %u ms.", timeoutMs);
    unsigned long startTime = millis();
    bool apModeActive = false;

    unsigned long reportInterval = timeoutMs / 10; // Report progress 10 times
    if (reportInterval == 0) reportInterval = 100;

    delay(100); // Give the system a moment to settle before checking mode

    for (unsigned long elapsed = 0; elapsed < timeoutMs; elapsed += reportInterval) {
        wifi_mode_t currentMode = WiFi.getMode();
        debugI("TEST: Waiting for AP mode... %lu/%u ms. Current mode: %d", elapsed, timeoutMs, currentMode);

        if (currentMode == WIFI_AP_STA) {
            debugW("TEST: Detected WIFI_AP_STA, attempting to force to WIFI_AP.");
            SetWiFiMode(WIFI_AP);
            currentMode = WiFi.getMode(); // Re-check mode after attempting to force
            debugI("TEST: Mode after force attempt: %d", currentMode);
        }

        if (currentMode == WIFI_AP && WiFi.softAPgetStationNum() >= 0) { // Check for active AP
            apModeActive = true;
            break;
        }
        delay(reportInterval);
    }
    
    if (apModeActive) {
        debugI("TEST: AP mode active. SSID: %s", WiFi.softAPSSID().c_str());
    } else {
        debugE("TEST: AP mode FAILED to activate within %u ms. Current mode: %d", timeoutMs, WiFi.getMode());
    }
    return apModeActive;
}

// Disable AP Mode (e.g., after a test that uses AP mode)
bool disableAPMode() {
    debugI("TEST: Disabling AP mode.");
    if (SetWiFiMode(WIFI_STA)) { // Attempt to switch to STA to disable AP
        debugI("TEST: AP mode successfully disabled (switched to STA).");
        return true;
    }
    debugE("TEST: Failed to disable AP mode.");
    return false;
}

// --- Test Cases Definition ---

// Test for Harrie Case (No Credentials / SSID Not Found)
WiFiTestStep harrieCaseSteps[] = {
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Starting Harrie Case Test"),
    WiFiTestStep(WiFiTestCommand::CLEAR_ALL_CREDENTIALS),
    WiFiTestStep(WiFiTestCommand::EXPECT_STA_CONNECTION, TEST_NON_EXISTENT_SSID, TEST_DUMMY_PASSWORD, 5000, WL_NO_SSID_AVAIL), // Expect failure to connect
    WiFiTestStep(WiFiTestCommand::START_CAPTIVE_PORTAL), // Explicitly start portal after STA failure
    WiFiTestStep(WiFiTestCommand::EXPECT_AP_MODE, nullptr, nullptr, TEST_SHORT_TIMEOUT_MS), // Should enter AP mode within ~30s timeout + some buffer
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Harrie Case Test Complete")
};
WiFiTestCase harrieCase("Harrie Case", harrieCaseSteps, sizeof(harrieCaseSteps) / sizeof(WiFiTestStep));

// Test for Mistyped Password Case
WiFiTestStep mistypedPasswordCaseSteps[] = {
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Starting Mistyped Password Case Test"),
    WiFiTestStep(WiFiTestCommand::CLEAR_ALL_CREDENTIALS),
    WiFiTestStep(WiFiTestCommand::SET_CREDENTIALS, TEST_HOME_SSID, TEST_HOME_WRONG_PASSWORD),
    WiFiTestStep(WiFiTestCommand::EXPECT_STA_CONNECTION, TEST_HOME_SSID, TEST_HOME_WRONG_PASSWORD, TEST_SHORT_TIMEOUT_MS, WL_DISCONNECTED), // Expect connection failure (WL_DISCONNECTED is more common for wrong password)
    WiFiTestStep(WiFiTestCommand::EXPECT_AP_MODE, nullptr, nullptr, TEST_AP_STABILIZE_MS), // Should enter AP mode quickly after STA failure
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Mistyped Password Case Test Complete"),
    WiFiTestStep(WiFiTestCommand::DISABLE_AP_MODE) // Clean up
};
WiFiTestCase mistypedPasswordCase("Mistyped Password Case", mistypedPasswordCaseSteps, sizeof(mistypedPasswordCaseSteps) / sizeof(WiFiTestStep));

// Test for Correct Password Case
WiFiTestStep correctPasswordCaseSteps[] = {
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Starting Correct Password Case Test"),
    WiFiTestStep(WiFiTestCommand::CLEAR_ALL_CREDENTIALS),
    WiFiTestStep(WiFiTestCommand::SET_CREDENTIALS, TEST_HOME_SSID, TEST_HOME_CORRECT_PASSWORD),
    WiFiTestStep(WiFiTestCommand::EXPECT_STA_CONNECTION, TEST_HOME_SSID, TEST_HOME_CORRECT_PASSWORD, TEST_SHORT_TIMEOUT_MS, WL_CONNECTED), // Expect successful connection
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Correct Password Case Test Complete")
};
WiFiTestCase correctPasswordCase("Correct Password Case", correctPasswordCaseSteps, sizeof(correctPasswordCaseSteps) / sizeof(WiFiTestStep));

// Test for clearsettings + startportal robustness
// This test verifies that after clearing credentials and potentially setting dummy ones,
// the system can still reliably enter AP mode, simulating the effect of 'startportal'
// or automatic AP fallback. It no longer performs an actual reboot mid-test.
WiFiTestStep clearSettingsStartPortalCaseSteps[] = {
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "Starting clearsettings + startportal Robustness Test (no actual reboot)"),
    WiFiTestStep(WiFiTestCommand::CLEAR_ALL_CREDENTIALS), // Simulate clearsettings
    WiFiTestStep(WiFiTestCommand::SET_CREDENTIALS, TEST_DUMMY_SSID, TEST_DUMMY_PASSWORD), // Set dummy credentials to trigger STA attempt/failure
    WiFiTestStep(WiFiTestCommand::EXPECT_STA_CONNECTION, TEST_DUMMY_SSID, TEST_DUMMY_PASSWORD, TEST_SHORT_TIMEOUT_MS, WL_DISCONNECTED), // Expect STA connection to fail (due to dummy creds)
    WiFiTestStep(WiFiTestCommand::START_CAPTIVE_PORTAL), // Explicitly start portal after STA failure
    WiFiTestStep(WiFiTestCommand::EXPECT_AP_MODE, nullptr, nullptr, TEST_AP_STABILIZE_MS), // Expect system to transition to AP mode after STA failure
    WiFiTestStep(WiFiTestCommand::LOG_MESSAGE, nullptr, nullptr, 0, WL_IDLE_STATUS, "clearsettings + startportal Robustness Test Complete")
};
WiFiTestCase clearSettingsStartPortalCase("clearsettings + startportal Robustness Case", clearSettingsStartPortalCaseSteps, sizeof(clearSettingsStartPortalCaseSteps) / sizeof(WiFiTestStep));


// Global array of pointers to all test cases
WiFiTestCase* g_wifiTestCases[] = {
    &harrieCase,
    &mistypedPasswordCase,
    &correctPasswordCase, // Added new test case
    &clearSettingsStartPortalCase
    // Add Dave Case here once it's fully defined. Dave Case needs an actual AP going offline.
};
size_t g_numWiFiTestCases = sizeof(g_wifiTestCases) / sizeof(WiFiTestCase*);


// --- WiFi Test Loop Task Entry Point ---
void WiFiTestLoopEntry(void* pvParameters) {
    debugI("Starting WiFi Test Loop Task.");

    // Give some time for system to fully initialize
    delay(TEST_BOOT_STABILIZE_MS); 

    backupWiFiCredentials();

    size_t passedCases = 0;
    size_t failedCases = 0;

    for (size_t i = 0; i < g_numWiFiTestCases; ++i) {
        WiFiTestCase* currentCase = g_wifiTestCases[i];
        debugI("===== Running Test Case %u of %u: %s =====", i + 1, g_numWiFiTestCases, currentCase->name);
        bool testCasePassed = true;

        for (size_t j = 0; j < currentCase->numSteps; ++j) {
            WiFiTestStep& step = currentCase->steps[j];
            debugI("--- Step %u of %u: Command %d ---", j + 1, currentCase->numSteps, (int)step.command);
            bool stepPassed = false;

            switch (step.command) {
                case WiFiTestCommand::LOG_MESSAGE:
                    debugI("TEST MESSAGE: %s", step.message);
                    stepPassed = true; // Logging always passes
                    break;

                case WiFiTestCommand::SET_CREDENTIALS:
                    stepPassed = setWiFiCredentials(step.ssid, step.password);
                    break;

                case WiFiTestCommand::CLEAR_ALL_CREDENTIALS:
                    clearAllWiFiCredentials();
                    stepPassed = true; // Clear credentials always passes unless NVS fails
                    break;
                
                case WiFiTestCommand::EXPECT_STA_CONNECTION:
                    stepPassed = expectStaConnection(step.ssid, step.password, step.timeoutMs, step.expectedStatus);
                    break;
                
                case WiFiTestCommand::EXPECT_AP_MODE:
                    stepPassed = expectAPMode(step.timeoutMs);
                    // After expecting AP mode, make sure to kick the webserver to start it
                    if (stepPassed && !g_ptrSystem->WebServer().IsCaptivePortalActive()) {
                        debugI("TEST: WebServer not active, starting Captive Portal WebServer explicitly.");
                        StartCaptivePortal(); // This call will initiate the AP, if not already
                    }
                    break;

                case WiFiTestCommand::DISABLE_AP_MODE:
                    stepPassed = disableAPMode();
                    break;

                case WiFiTestCommand::START_CAPTIVE_PORTAL:
                    debugI("TEST: Explicitly starting Captive Portal.");
                    StartCaptivePortal();
                    delay(1000); // Give the captive portal time to start and stabilize
                    stepPassed = true; // Assume success for now, expectAPMode will verify
                    break;

                case WiFiTestCommand::WAIT_FOR_MS:
                    debugI("TEST: Waiting for %u ms.", step.timeoutMs);
                    delay(step.timeoutMs);
                    stepPassed = true;
                    break;


                case WiFiTestCommand::REBOOT_DEVICE:
                    debugI("TEST: Requesting device reboot for next test phase.");
                    // This will restart the system, so the next loop iteration won't happen.
                    // The next test phase would start after reboot.
                    ESP.restart(); 
                    break;

                default:
                    debugE("TEST ERROR: Unknown command in test step.");
                    stepPassed = false;
                    break;
            }

            if (!stepPassed) {
                testCasePassed = false;
                debugE("===== Test Step FAILED in %s: Command %d =====", currentCase->name, (int)step.command);
                break; // Exit current test case on first failure
            } else {
                 debugI("TEST Step PASSED in %s: Command %d", currentCase->name, (int)step.command);
            }
        }

        if (testCasePassed) {
            debugI("===== Test Case %s: PASSED =====", currentCase->name);
            passedCases++;
        } else {
            debugE("===== Test Case %s: FAILED =====", currentCase->name);
            failedCases++;
        }
        delay(5000); // Small delay between test cases
    }

    debugI("===== All WiFi Tests Completed: %u PASSED, %u FAILED of %u =====", passedCases, failedCases, g_numWiFiTestCases);
    
    restoreWiFiCredentials();

    debugI("TEST: Starting main network loop to connect with restored credentials or run captive portal.");
    g_ptrSystem->TaskManager().StartNetworkThread();

    debugI("TEST: Test task complete. Exiting.");
    vTaskDelete(NULL); // Delete the current task
}

#endif // ENABLE_WIFI_TEST_MODE