#ifndef VALIDATION_UTILS_H
#define VALIDATION_UTILS_H

#include <Arduino.h>

// Utility functions for input validation and sanitization
class ValidationUtils {
public:
    // Validate JSON payload size to prevent memory exhaustion
    static bool validateJSONSize(size_t len, size_t maxSize = 2048);

    // Sanitize string input by removing control characters and trimming
    static String sanitizeString(const String& input);

    // Validate port number (1-65535)
    static bool isValidPort(uint16_t port);

    // Validate IP address string format
    static bool isValidIPAddress(const String& ip);

    // Validate SSID length and characters
    static bool isValidSSID(const String& ssid);

    // Validate password length
    static bool isValidPassword(const String& password);

    // Check if string contains only printable ASCII characters
    static bool isPrintableASCII(const String& str);

private:
    ValidationUtils() {} // Static utility class
};

#endif // VALIDATION_UTILS_H