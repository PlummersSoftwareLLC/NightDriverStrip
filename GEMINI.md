## WiFi AP Robustness Project Summary

This project aimed to significantly improve the stability and reliability of the device's WiFi Access Point (AP) mode, particularly concerning captive portal activation and transitions between Station (STA) and AP modes. The work addressed several key issues, including long timeouts when a known WiFi network is unavailable and system instability ("wedging") when forcing the device into AP mode.

### Key Problems Addressed:

1.  **Long Captive Portal Timeout:** When a previously configured WiFi network was temporarily unavailable, the device would try to reconnect for up to 15 minutes (`900` seconds) before starting the captive portal, leading to a poor user experience. This is referred to as the "Dave Case".
2.  **Quick Feedback for Wrong Credentials:** When a user entered the wrong password for a known SSID, the device would also wait for the long timeout. It should fail fast and allow the user to re-enter credentials. This is the "Mistyped Password Case".
3.  **No Credentials / SSID Not Found:** On first-time setup or when no known SSIDs are found, the device should quickly enter captive portal mode. This is the "Harrie Case".
4.  **System Instability and Memory Leaks:** Rapidly or repeatedly attempting to force the device into AP mode (e.g., using `clearsettings` then `startportal`, or after repeated connection failures) could cause the WiFi stack to become unresponsive and lead to significant memory leaks, eventually crashing the device.

### Solutions Implemented:

To address these issues, the following changes were made:

1.  **Smarter Timeouts:** The WiFi connection logic in `src/network.cpp` was updated to use a short timeout (`30` seconds) for cases where connection fails immediately due to `WL_NO_SSID_AVAIL` or `WL_CONNECT_FAILED` (Harrie and Mistyped Password cases), while retaining the long timeout for temporary disconnections (Dave case).
2.  **Robust WiFi Mode Switching:** A new function, `SetWiFiModeRobustly()`, was implemented in `include/network.h` and `src/network.cpp`. This function ensures clean transitions between WiFi modes by explicitly disconnecting, setting the new mode, and polling to confirm the change. It is now used by all functions that initiate the captive portal.
3.  **Fixes for AP Mode Activation:** Several specific bugs were fixed, including handling implicit mode changes caused by `WiFi.scanNetworks()` and ensuring the device reliably enters `WIFI_AP` mode.
4.  **Automated Testing Framework:** A comprehensive, on-device automated testing framework was created (`src/wifi_test.cpp`, `include/wifi_test.h`, `include/wifi_test_config.h`) to continuously verify the correctness of the WiFi state machine and prevent regressions. These tests cover all the cases mentioned above.
5.  **Memory Leak and Stability Fixes:**
    *   Resolved a memory leak caused by repeated, failing attempts to start the captive portal. The fix ensures that the "captive portal active" state is set *before* attempting to change the WiFi mode, preventing a loop of failed attempts that exhausted memory.
    *   Ensured thread safety by confirming the use of `std::atomic<bool>` for the `servicesStarted` flag, which is accessed by multiple tasks.
    *   Optimized memory by pre-allocating capacity for the `_availableNetworks` vector before scanning for WiFi networks, preventing multiple reallocations.

As a result of these changes, all automated WiFi test cases now pass, ensuring reliable WiFi mode transitions and a more robust captive portal experience.

---

Documenting Lessons Learned:

   1. Effective Debugging with `tio.log` and `nc`:
       * Problem: Difficulty observing device serial output directly.
       * Solution: Use ./go (build, flash, tio to tio.log) and then read tio.log via cat or tail -f.
       * Commands: ./go, cat tio.log, tail -n 1000 -f tio.log, (echo "command"; sleep 1) | nc -w 5 <device_ip> 23 for RemoteDebug
         commands.
       * Benefit: Direct device observation, reduced user burden.

   2. Reliable RemoteDebug Command Execution:
       * Problem: Unreliable nc command sending.
       * Solution: (echo "command"; sleep 1) | telnet <device_ip> 23 for robust command delivery.
       * Benefit: Ensures reliable execution of debug commands.

   3. Captive Portal & Web UI Debugging:
       * Discovery: /statistics endpoint provides rich JSON system metrics.
       * Benefit: Programmatic access to structured debug data via web_fetch (using direct IP).

   4. Addressing Agent Communication Failures:
       * Problem: Agent misinterpreting "System: Please continue" as user input, leading to loops.
       * Lesson: Need robust internal logic to differentiate system prompts from user input and manage "awaiting user input" state
         effectively.

## WiFi State Management Cases & Test Plan Guidelines

This section describes key WiFi state management scenarios in the NightDriver system, outlining expected behavior and providing guidance for testing. These cases inform the `AUTO` WiFi mode logic and help ensure a robust user experience.

### 1. WiFi State Management Cases

#### The Harrie Case (First-Time Setup / SSID Not Found)
*   **Scenario:** The device has no stored WiFi credentials, or it's attempting to connect to a network whose SSID is not currently visible or available. This typically happens during first-time setup or when the device is moved to a new location without known networks.
*   **Purpose:** To quickly provide a mechanism for the user to configure WiFi credentials without a long wait.
*   **Expected Behavior:** The device will attempt to connect for a `AUTO_MODE_SHORT_TIMEOUT_SECONDS` (e.g., 30 seconds) period. If unsuccessful (e.g., `WL_NO_SSID_AVAIL`), it will quickly transition into Access Point (AP) mode, hosting a Captive Portal for configuration.
*   **Triggering Conditions (Internal):** `WiFi.status()` returns `WL_NO_SSID_AVAIL` (integer `1`).

#### The Dave Case (Known Network Temporarily Unavailable)
*   **Scenario:** The device has previously successfully connected to a WiFi network. However, that network is currently not reachable (e.g., router rebooting, temporary outage) but is expected to return.
*   **Purpose:** To patiently wait for the previously connected network to become available again, avoiding unnecessary re-configuration.
*   **Expected Behavior:** The device will continuously retry connecting to the known network for a `AUTO_MODE_LONG_TIMEOUT_SECONDS` (e.g., 900 seconds / 15 minutes) period. Only after this extended timeout will it transition into AP mode with the Captive Portal.
*   **Triggering Conditions (Internal):** `WiFi.status()` indicates a disconnection (`WL_DISCONNECTED`, `WL_CONNECTION_LOST`, etc.) but *not* `WL_NO_SSID_AVAIL` or `WL_CONNECT_FAILED`.

#### The Mistyped Password Case (Connection Failed to Known Network)
*   **Scenario:** The device attempts to connect to a previously known (or manually entered) WiFi network, but the connection fails due to incorrect credentials (e.g., wrong password, authentication error). The SSID *is* visible, but authentication fails.
*   **Purpose:** To provide quick feedback to the user that the entered credentials are incorrect, allowing them to re-enter the correct password without waiting for the long "Dave Case" timeout.
*   **Expected Behavior:** The device will attempt to connect for a `AUTO_MODE_SHORT_TIMEOUT_SECONDS` (e.g., 30 seconds) period. If unsuccessful due to connection failure (`WL_CONNECT_FAILED`), it will quickly transition into AP mode, hosting a Captive Portal for re-configuration.
*   **Triggering Conditions (Internal):** `WiFi.status()` returns `WL_CONNECT_FAILED` (integer `4`).

#### The "clearsettings + startportal" Robustness Case (AP Mode Transition)
*   **Scenario:** The device's settings are cleared using the `clearsettings` debug command (which triggers a reboot), and immediately after reboot, the `startportal` debug command is issued (or the system naturally falls into AP mode after the short timeout). Historically, this sequence could lead to a "wedged" state where the WiFi stack becomes unresponsive.
*   **Purpose:** To ensure that forceful and rapid transitions between WiFi operational modes (especially from `WIFI_OFF` to `WIFI_AP`) are handled gracefully and robustly.
*   **Expected Behavior:** The device should reliably transition into AP mode and successfully start the Captive Portal, allowing for configuration. The system should not "wedge" or become unresponsive.
*   **Mitigation:** The `SetWiFiModeRobustly` function has been introduced to ensure stable mode transitions by including necessary delays and polling for mode changes.

### 2. Test Plan Guidelines

The following guidelines aim to verify the expected behaviors of the WiFi state machine without requiring excessive manual interaction or waiting.

*   **Tools:**
    *   **Serial Monitor / Telnet:** To observe debug logs (`debugI`, `debugW`, `debugE`) and issue RemoteDebug commands (`clearsettings`, `startportal`, `showWiFiState`, `forceWiFiState auto`).
    *   **Mobile Device / Laptop:** To connect to the device's Captive Portal AP and interact with the web UI.
    *   **WiFi Network Configuration:** Ability to control SSID visibility and password for an existing access point.

*   **General Principle:** Use `forceWiFiState auto` and `clearsettings` to set the desired initial state for testing "Harrie," "Dave," and "Mistyped Password" cases. Use `startportal` to test immediate AP mode transitions.

---

#### Test Case 1: The Harrie Case (No Credentials / SSID Not Found)

*   **Preconditions:**
    1.  Device has no stored WiFi credentials. (Achieve by `clearsettings` + reboot).
    2.  No known WiFi networks are visible in the vicinity (e.g., physically disable your router's WiFi, or test in a location with no known networks).
    3.  Device is running the modified firmware.

*   **Steps:**
    1.  Connect to the device via Serial Monitor or Telnet.
    2.  Issue `clearsettings` command. Device will reboot.
    3.  Observe debug logs after reboot.
    4.  Wait for `AUTO_MODE_SHORT_TIMEOUT_SECONDS` (30 seconds).

*   **Expected Result:**
    1.  After `clearsettings`, device reboots and attempts to connect in STA mode.
    2.  Logs indicate `WL_NO_SSID_AVAIL` or similar connection failure, and repeated attempts to connect.
    3.  After approx. 30 seconds, logs indicate transition to AP mode and the Captive Portal starts (`debugI("Starting Captive Portal AP setup.");`).
    4.  A new WiFi access point named "NightDriver-Setup-XXXXXX" should appear (where XXXXXX is part of the MAC address).
    5.  You should be able to connect to this AP and access the Captive Portal.

*   **Verification:** Confirm AP appears, connect a client, verify Captive Portal loads.

---

#### Test Case 2: The Dave Case (Known Network Temporarily Unavailable)

*   **Preconditions:**
    1.  Device has valid credentials for a known WiFi network (e.g., your home WiFi).
    2.  Device has successfully connected to this network at least once. (Achieve by connecting to home WiFi via Captive Portal, then reboot).
    3.  The known WiFi network is *temporarily* unavailable (e.g., disable broadcast/turn off WiFi on your router for a short period, but keep the router powered).
    4.  Device is running the modified firmware.

*   **Steps:**
    1.  Connect to device via Captive Portal, provide correct credentials for your home WiFi.
    2.  Let device reboot and successfully connect to your home WiFi (verify via logs or `showWiFiState`).
    3.  Disconnect the device from Serial/Telnet.
    4.  Temporarily disable your home WiFi broadcast/turn off WiFi on your router.
    5.  Reconnect to Serial Monitor/Telnet (if possible via the network, otherwise via USB-serial).
    6.  Observe debug logs. Device should be in `AUTO` mode.
    7.  Wait for `AUTO_MODE_LONG_TIMEOUT_SECONDS` (15 minutes).

*   **Expected Result:**
    1.  Device attempts to reconnect to the known SSID, logging `WL_DISCONNECTED` or similar, but *not* `WL_NO_SSID_AVAIL` or `WL_CONNECT_FAILED`.
    2.  It continues retrying for approximately 15 minutes.
    3.  Only after 15 minutes, logs indicate transition to AP mode and the Captive Portal starts.
    4.  A new WiFi access point named "NightDriver-Setup-XXXXXX" should appear.

*   **Verification:** Confirm the extended wait period in logs before AP appears. Use `showWiFiState` or `forceWiFiState auto` (if needed) to ensure the device is in auto mode if it gets stuck.

---

#### Test Case 3: The Mistyped Password Case

*   **Preconditions:**
    1.  Device has valid credentials for a known WiFi network (your home WiFi).
    2.  Device is running the modified firmware.

*   **Steps:**
    1.  Connect to the device via Serial Monitor or Telnet.
    2.  Issue `clearsettings` command. Device will reboot.
    3.  Wait for the device to enter Captive Portal mode (Harrie Case).
    4.  Connect a mobile device/laptop to the "NightDriver-Setup-XXXXXX" AP.
    5.  Access the Captive Portal.
    6.  Enter the correct SSID for your home WiFi, but intentionally enter an *incorrect* password.
    7.  Submit the form. Device will reboot.
    8.  Reconnect to Serial Monitor/Telnet.
    9.  Observe debug logs.

*   **Expected Result:**
    1.  After reboot, device attempts to connect to your home WiFi with the incorrect password.
    2.  Logs indicate `WL_CONNECT_FAILED` (integer `4`) or similar authentication failure, and repeated attempts.
    3.  After approx. `AUTO_MODE_SHORT_TIMEOUT_SECONDS` (30 seconds), logs indicate transition to AP mode and the Captive Portal starts.
    4.  The "NightDriver-Setup-XXXXXX" AP reappears.

*   **Verification:** Confirm the shorter wait period in logs before AP reappears.

---

#### Test Case 4: "clearsettings + startportal" Robustness

*   **Preconditions:**
    1.  Device is running the modified firmware with `SetWiFiModeRobustly` implemented.

*   **Steps:**
    1.  Connect to the device via Serial Monitor or Telnet.
    2.  Issue `clearsettings` command. Device will reboot.
    3.  *Immediately* after the device finishes its boot sequence (i.e., you see the initial debug output from `main.cpp` and `network.cpp` indicating STA connection attempts), issue the `startportal` command.

*   **Expected Result:**
    1.  The device should gracefully transition into AP mode.
    2.  Logs should show `debugI("Attempting to set WiFi mode to WIFI_AP")` from `SetWiFiModeRobustly`.
    3.  The "NightDriver-Setup-XXXXXX" AP should appear.
    4.  The Captive Portal should load successfully when a client connects to the AP.
    5.  The device should *not* "wedge" or become unresponsive.

*   **Verification:** Confirm AP appears and Captive Portal loads. Check logs for any unexpected errors or hangs. If it works, try repeating the test multiple times to ensure consistency.

---