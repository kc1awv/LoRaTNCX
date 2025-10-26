#include "DisplayManager.h"

DisplayManager::ScreenState DisplayManager::screens[8];
uint8_t DisplayManager::globalDirtyFlags = 0xFF;  // Start with all dirty
unsigned long DisplayManager::lastActivity[8] = {0};

void DisplayManager::init() {
    for (int i = 0; i < 8; i++) {
        screens[i].isDirty = true;
        screens[i].lastUpdate = 0;
        screens[i].dataHash = 0;
        lastActivity[i] = millis();
    }
    globalDirtyFlags = 0xFF;
}

void DisplayManager::markDirty(uint8_t flags) {
    globalDirtyFlags |= flags;
    
    // Mark relevant screens dirty based on flags
    if (flags & BATTERY_CHANGED) {
        screens[0].isDirty = true;  // Status screen
        screens[6].isDirty = true;  // Power screen
    }
    
    if (flags & WIFI_CHANGED) {
        screens[0].isDirty = true;  // Status screen
        screens[4].isDirty = true;  // Network screen
    }
    
    if (flags & RADIO_CHANGED) {
        screens[1].isDirty = true;  // Radio screen
        screens[5].isDirty = true;  // Stats screen
    }
    
    if (flags & GNSS_CHANGED) {
        screens[2].isDirty = true;  // GNSS screen
        screens[3].isDirty = true;  // APRS screen
    }
    
    if (flags & STATS_CHANGED) {
        screens[5].isDirty = true;  // Stats screen
    }
    
    if (flags & (SCREEN_CHANGED | FORCE_UPDATE)) {
        for (int i = 0; i < 8; i++) {
            screens[i].isDirty = true;
        }
    }
}

void DisplayManager::markScreenDirty(int screenId) {
    if (screenId >= 0 && screenId < 8) {
        screens[screenId].isDirty = true;
    }
}

bool DisplayManager::needsUpdate(int screenId, unsigned long minInterval) {
    if (screenId < 0 || screenId >= 8) return false;
    
    ScreenState& screen = screens[screenId];
    unsigned long now = millis();
    
    // Check if enough time has passed
    if (minInterval > 0 && (now - screen.lastUpdate) < minInterval) {
        return false;
    }
    
    // Check if screen is dirty
    return screen.isDirty;
}

void DisplayManager::updated(int screenId) {
    if (screenId >= 0 && screenId < 8) {
        screens[screenId].isDirty = false;
        screens[screenId].lastUpdate = millis();
    }
}

bool DisplayManager::hasChanged(int screenId, uint32_t newHash) {
    if (screenId < 0 || screenId >= 8) return true;
    
    ScreenState& screen = screens[screenId];
    if (screen.dataHash != newHash) {
        screen.dataHash = newHash;
        screen.isDirty = true;
        return true;
    }
    
    return false;
}

unsigned long DisplayManager::getRefreshInterval(int screenId) {
    if (screenId < 0 || screenId >= 8) return NORMAL_REFRESH_MS;
    
    unsigned long now = millis();
    unsigned long timeSinceActivity = now - lastActivity[screenId];
    
    // Fast refresh if recent activity
    if (timeSinceActivity < 5000) {
        return FAST_REFRESH_MS;
    }
    
    // Slow refresh if no recent activity
    if (timeSinceActivity > ACTIVITY_TIMEOUT_MS) {
        return SLOW_REFRESH_MS;
    }
    
    return NORMAL_REFRESH_MS;
}

void DisplayManager::recordActivity(int screenId) {
    if (screenId >= 0 && screenId < 8) {
        lastActivity[screenId] = millis();
    }
}

// Simple hash function for change detection
uint32_t DisplayManager::calculateHash(const void* data, size_t len) {
    uint32_t hash = 5381;
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }
    
    return hash;
}

DisplayManager displayManager;