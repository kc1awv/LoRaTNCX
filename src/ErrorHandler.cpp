#include "ErrorHandler.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

ErrorHandler::ErrorInfo ErrorHandler::errors[10];
uint32_t ErrorHandler::watchdogTimeout = 30000;
unsigned long ErrorHandler::lastWatchdogFeed = 0;
bool ErrorHandler::watchdogEnabled = false;

void ErrorHandler::init() {
    // Initialize error tracking array
    for (int i = 0; i < 10; i++) {
        errors[i].type = static_cast<ErrorType>(i);
        errors[i].description = getErrorDescription(static_cast<ErrorType>(i));
        errors[i].timestamp = 0;
        errors[i].count = 0;
        errors[i].lastAction = RECOVERY_NONE;
        errors[i].resolved = true;
    }
    
    Serial.println("[ERROR] Error handler initialized");
}

bool ErrorHandler::reportError(ErrorType type, const char* context) {
    if (type >= 10) return false;
    
    ErrorInfo& error = errors[type];
    error.timestamp = millis();
    error.count++;
    error.resolved = false;
    
    logError(type, context);
    
    // Attempt automatic recovery for non-critical errors
    if (type != ERROR_CRITICAL) {
        return attemptRecovery(type);
    }
    
    return false;
}

void ErrorHandler::resolveError(ErrorType type) {
    if (type < 10) {
        errors[type].resolved = true;
        Serial.printf("[ERROR] Error %d resolved\n", type);
    }
}

bool ErrorHandler::hasError(ErrorType type) {
    return (type < 10) ? !errors[type].resolved : false;
}

bool ErrorHandler::hasCriticalError() {
    return !errors[ERROR_CRITICAL].resolved;
}

const ErrorHandler::ErrorInfo* ErrorHandler::getErrorInfo(ErrorType type) {
    return (type < 10) ? &errors[type] : nullptr;
}

void ErrorHandler::clearAllErrors() {
    for (int i = 0; i < 10; i++) {
        errors[i].resolved = true;
        errors[i].count = 0;
    }
    Serial.println("[ERROR] All errors cleared");
}

bool ErrorHandler::attemptRecovery(ErrorType type) {
    if (type >= 10) return false;
    
    ErrorInfo& error = errors[type];
    RecoveryAction action = getRecoveryAction(type);
    error.lastAction = action;
    
    Serial.printf("[ERROR] Attempting recovery for error %d with action %d\n", type, action);
    
    switch (action) {
        case RECOVERY_RETRY:
            // Simple retry - let calling code handle specifics
            return true;
            
        case RECOVERY_RESTART:
            Serial.println("[ERROR] Performing system restart for recovery");
            delay(1000);
            ESP.restart();
            return false;  // Never reached
            
        case RECOVERY_FALLBACK:
            Serial.printf("[ERROR] Using fallback mode for error %d\n", type);
            return true;
            
        case RECOVERY_DISABLE:
            Serial.printf("[ERROR] Disabling component for error %d\n", type);
            return false;
            
        default:
            return false;
    }
}

void ErrorHandler::performWatchdogReset() {
    Serial.println("[ERROR] Watchdog reset triggered");
    esp_restart();
}

void ErrorHandler::checkMemoryHealth() {
    size_t freeHeap = ESP.getFreeHeap();
    size_t minHeap = ESP.getMinFreeHeap();
    
    // Trigger error if free heap is very low
    if (freeHeap < 10240) {  // Less than 10KB
        reportError(ERROR_MEMORY_LOW, "Free heap critically low");
    }
    
    // Log memory status periodically
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck > 60000) {  // Every minute
        Serial.printf("[MEM] Free: %d bytes, Min: %d bytes\n", freeHeap, minHeap);
        lastMemCheck = millis();
    }
}

void ErrorHandler::enableWatchdog(uint32_t timeoutMs) {
    watchdogTimeout = timeoutMs;
    watchdogEnabled = true;
    lastWatchdogFeed = millis();
    
    // Check if watchdog is already initialized
    esp_err_t wdt_status = esp_task_wdt_status(NULL);
    if (wdt_status == ESP_OK) {
        // Watchdog already initialized, just add our task
        Serial.println("[WDT] Watchdog already initialized, adding current task");
        esp_task_wdt_add(NULL);
    } else {
        // Initialize watchdog with compatible API
        esp_err_t init_result = esp_task_wdt_init(timeoutMs / 1000, true); // timeout in seconds, panic on timeout
        if (init_result == ESP_OK) {
            esp_task_wdt_add(NULL);  // Add current task
            Serial.printf("[WDT] Watchdog initialized with %lu ms timeout\n", timeoutMs);
        } else {
            Serial.printf("[WDT] Failed to initialize watchdog: %s\n", esp_err_to_name(init_result));
            watchdogEnabled = false;
        }
    }
}

void ErrorHandler::feedWatchdog() {
    if (watchdogEnabled) {
        lastWatchdogFeed = millis();
        esp_task_wdt_reset();
    }
}

void ErrorHandler::printErrorStatus() {
    Serial.println("[ERROR] === Error Status Report ===");
    
    uint16_t activeErrors = 0;
    for (int i = 0; i < 10; i++) {
        ErrorInfo& error = errors[i];
        if (!error.resolved) {
            Serial.printf("[ERROR] %d: %s (Count: %d, Time: %lu)\n", 
                         i, error.description, error.count, error.timestamp);
            activeErrors++;
        }
    }
    
    if (activeErrors == 0) {
        Serial.println("[ERROR] No active errors");
    } else {
        Serial.printf("[ERROR] %d active errors\n", activeErrors);
    }
}

String ErrorHandler::getErrorSummary() {
    uint16_t activeErrors = 0;
    String summary = "Errors: ";
    
    for (int i = 0; i < 10; i++) {
        if (!errors[i].resolved) {
            activeErrors++;
        }
    }
    
    if (activeErrors == 0) {
        summary += "None";
    } else {
        summary += String(activeErrors) + " active";
    }
    
    return summary;
}

uint16_t ErrorHandler::getTotalErrorCount() {
    uint16_t total = 0;
    for (int i = 0; i < 10; i++) {
        total += errors[i].count;
    }
    return total;
}

void ErrorHandler::logError(ErrorType type, const char* context) {
    Serial.printf("[ERROR] Type: %d, Description: %s", type, getErrorDescription(type));
    if (context) {
        Serial.printf(", Context: %s", context);
    }
    Serial.printf(", Count: %d\n", errors[type].count);
}

const char* ErrorHandler::getErrorDescription(ErrorType type) {
    switch (type) {
        case ERROR_NONE: return "No error";
        case ERROR_RADIO_INIT: return "Radio initialization failed";
        case ERROR_DISPLAY_INIT: return "Display initialization failed";
        case ERROR_WIFI_CONNECT: return "WiFi connection failed";
        case ERROR_GNSS_TIMEOUT: return "GNSS timeout";
        case ERROR_BATTERY_READ: return "Battery read failed";
        case ERROR_CONFIG_LOAD: return "Configuration load failed";
        case ERROR_MEMORY_LOW: return "Memory critically low";
        case ERROR_WATCHDOG: return "Watchdog timeout";
        case ERROR_CRITICAL: return "Critical system error";
        default: return "Unknown error";
    }
}

ErrorHandler::RecoveryAction ErrorHandler::getRecoveryAction(ErrorType type) {
    switch (type) {
        case ERROR_RADIO_INIT: return RECOVERY_RETRY;
        case ERROR_DISPLAY_INIT: return RECOVERY_FALLBACK;
        case ERROR_WIFI_CONNECT: return RECOVERY_FALLBACK;
        case ERROR_GNSS_TIMEOUT: return RECOVERY_RETRY;
        case ERROR_BATTERY_READ: return RECOVERY_RETRY;
        case ERROR_CONFIG_LOAD: return RECOVERY_FALLBACK;
        case ERROR_MEMORY_LOW: return RECOVERY_RESTART;
        case ERROR_WATCHDOG: return RECOVERY_RESTART;
        case ERROR_CRITICAL: return RECOVERY_RESTART;
        default: return RECOVERY_NONE;
    }
}

ErrorHandler errorHandler;