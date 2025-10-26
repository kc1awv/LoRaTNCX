#pragma once
#include <Arduino.h>

// Centralized error handling and recovery system
class ErrorHandler 
{
public:
    enum ErrorType {
        ERROR_NONE = 0,
        ERROR_RADIO_INIT = 1,
        ERROR_DISPLAY_INIT = 2,
        ERROR_WIFI_CONNECT = 3,
        ERROR_GNSS_TIMEOUT = 4,
        ERROR_BATTERY_READ = 5,
        ERROR_CONFIG_LOAD = 6,
        ERROR_MEMORY_LOW = 7,
        ERROR_WATCHDOG = 8,
        ERROR_CRITICAL = 9
    };

    enum RecoveryAction {
        RECOVERY_NONE = 0,
        RECOVERY_RETRY = 1,
        RECOVERY_RESTART = 2,
        RECOVERY_FALLBACK = 3,
        RECOVERY_DISABLE = 4
    };

    struct ErrorInfo {
        ErrorType type;
        const char* description;
        unsigned long timestamp;
        uint16_t count;
        RecoveryAction lastAction;
        bool resolved;
    };

    static void init();
    static bool reportError(ErrorType type, const char* context = nullptr);
    static void resolveError(ErrorType type);
    static bool hasError(ErrorType type);
    static bool hasCriticalError();
    static const ErrorInfo* getErrorInfo(ErrorType type);
    static void clearAllErrors();
    
    // Recovery functions
    static bool attemptRecovery(ErrorType type);
    static void performWatchdogReset();
    static void checkMemoryHealth();
    static void enableWatchdog(uint32_t timeoutMs = 30000);
    static void feedWatchdog();
    
    // Error reporting
    static void printErrorStatus();
    static String getErrorSummary();
    static uint16_t getTotalErrorCount();
    
private:
    static ErrorInfo errors[10];  // Array for each error type
    static uint32_t watchdogTimeout;
    static unsigned long lastWatchdogFeed;
    static bool watchdogEnabled;
    
    // Internal recovery methods
    static bool retryOperation(ErrorType type, std::function<bool()> operation, int maxRetries = 3);
    static void logError(ErrorType type, const char* context);
    static const char* getErrorDescription(ErrorType type);
    static RecoveryAction getRecoveryAction(ErrorType type);
};

// Convenience macros for error handling
#define HANDLE_ERROR(type, context) ErrorHandler::reportError(type, context)
#define CHECK_ERROR(type) ErrorHandler::hasError(type)
#define RESOLVE_ERROR(type) ErrorHandler::resolveError(type)
#define FEED_WATCHDOG() ErrorHandler::feedWatchdog()

extern ErrorHandler errorHandler;