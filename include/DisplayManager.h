#pragma once
#include <Arduino.h>

// Display state management for efficient updates
class DisplayManager 
{
public:
    struct ScreenState {
        bool isDirty = true;
        unsigned long lastUpdate = 0;
        uint32_t dataHash = 0;  // Hash of current data for change detection
    };

    enum DirtyFlag {
        SCREEN_CHANGED = 0x01,
        BATTERY_CHANGED = 0x02,
        WIFI_CHANGED = 0x04,
        RADIO_CHANGED = 0x08,
        GNSS_CHANGED = 0x10,
        STATS_CHANGED = 0x20,
        FORCE_UPDATE = 0x80
    };

    static void init();
    static void markDirty(uint8_t flags);
    static void markScreenDirty(int screenId);
    static bool needsUpdate(int screenId, unsigned long minInterval = 0);
    static void updated(int screenId);
    static bool hasChanged(int screenId, uint32_t newHash);
    
    // Adaptive refresh rates based on activity
    static unsigned long getRefreshInterval(int screenId);
    static void recordActivity(int screenId);
    
private:
    static ScreenState screens[8];  // One for each screen type
    static uint8_t globalDirtyFlags;
    static unsigned long lastActivity[8];
    static const unsigned long FAST_REFRESH_MS = 500;
    static const unsigned long NORMAL_REFRESH_MS = 2000;
    static const unsigned long SLOW_REFRESH_MS = 5000;
    static const unsigned long ACTIVITY_TIMEOUT_MS = 30000;
    
    static uint32_t calculateHash(const void* data, size_t len);
};

extern DisplayManager displayManager;