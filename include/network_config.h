#pragma once

#ifdef ENABLE_WIFI_TEST_MODE
    // Use short timeouts for testing
    #define AUTO_MODE_SHORT_TIMEOUT_SECONDS 15
    #define AUTO_MODE_LONG_TIMEOUT_SECONDS 30
#else
    // Production values
    #define AUTO_MODE_SHORT_TIMEOUT_SECONDS 30
    #define AUTO_MODE_LONG_TIMEOUT_SECONDS 900
#endif
