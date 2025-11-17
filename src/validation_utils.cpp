#include "validation_utils.h"

bool ValidationUtils::validateJSONSize(size_t len, size_t maxSize) {
    return len > 0 && len <= maxSize;
}

String ValidationUtils::sanitizeString(const String& input) {
    String sanitized = input;

    // Remove any control characters (keep only printable ASCII)
    for (size_t i = 0; i < sanitized.length(); ) {
        if (sanitized[i] < 32 || sanitized[i] > 126) {
            sanitized.remove(i, 1);
        } else {
            i++;
        }
    }

    // Trim whitespace
    sanitized.trim();

    return sanitized;
}

bool ValidationUtils::isValidPort(uint16_t port) {
    return port > 0 && port <= 65535;
}

bool ValidationUtils::isValidIPAddress(const String& ip) {
    // Basic IP address validation (xxx.xxx.xxx.xxx format)
    if (ip.length() < 7 || ip.length() > 15) {
        return false;
    }

    int dots = 0;
    for (size_t i = 0; i < ip.length(); i++) {
        char c = ip[i];
        if (c == '.') {
            dots++;
        } else if (!isdigit(c)) {
            return false;
        }
    }

    return dots == 3;
}

bool ValidationUtils::isValidSSID(const String& ssid) {
    if (ssid.length() < 1 || ssid.length() > 32) {
        return false;
    }

    // Check for printable ASCII characters only
    return isPrintableASCII(ssid);
}

bool ValidationUtils::isValidPassword(const String& password) {
    // WPA2 passwords can be 8-63 characters
    return password.length() >= 8 && password.length() <= 63 && isPrintableASCII(password);
}

bool ValidationUtils::isPrintableASCII(const String& str) {
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c < 32 || c > 126) {
            return false;
        }
    }
    return true;
}