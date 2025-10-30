#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLOCATION(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Location: " + String(config.latitude, 6) + ", " + 
                     String(config.longitude, 6) + ", " + String(config.altitude) + "m");
        return TNCCommandResult::SUCCESS;
    }
    
    if (argCount < 2) {
        sendResponse("Usage: LOCATION <latitude> <longitude> [altitude]");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    float lat = args[0].toFloat();
    float lon = args[1].toFloat();
    int alt = (argCount > 2) ? args[2].toInt() : 0;
    
    if (lat < -90.0 || lat > 90.0) {
        sendResponse("ERROR: Latitude must be -90.0 to 90.0");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    if (lon < -180.0 || lon > 180.0) {
        sendResponse("ERROR: Longitude must be -180.0 to 180.0");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.latitude = lat;
    config.longitude = lon;
    config.altitude = alt;
    
    sendResponse("Location set to: " + String(lat, 6) + ", " + String(lon, 6) + ", " + String(alt) + "m");
    return TNCCommandResult::SUCCESS;
}
