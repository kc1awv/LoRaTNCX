#include "CommandContext.h"
#include "SystemLogger.h"

TNCCommandResult TNCCommands::handleLOG(const String args[], int argCount) {
    SystemLogger* logger = SystemLogger::getInstance();
    if (!logger) {
        sendResponse("Error: Logging system not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    // Default values
    size_t count = 50;
    LogLevel minLevel = LogLevel::INFO;
    bool showStats = false;
    bool clearLog = false;
    
    // Parse arguments
    for (int i = 0; i < argCount; i++) {
        String arg = args[i];
        arg.toUpperCase();
        
        if (arg.startsWith("COUNT=") || arg.startsWith("N=")) {
            String countStr = arg.substring(arg.indexOf('=') + 1);
            int parsedCount = countStr.toInt();
            if (parsedCount > 0 && parsedCount <= 1000) {
                count = parsedCount;
            } else {
                sendResponse("Error: Count must be between 1 and 1000");
                return TNCCommandResult::ERROR_INVALID_PARAMETER;
            }
        }
        else if (arg.startsWith("LEVEL=")) {
            String levelStr = arg.substring(6);  // Skip "LEVEL="
            minLevel = SystemLogger::stringToLevel(levelStr);
        }
        else if (arg == "STATS") {
            showStats = true;
        }
        else if (arg == "CLEAR") {
            clearLog = true;
        }
        else if (arg == "ALL") {
            count = 0;  // Get all entries
        }
        else if (arg == "DEBUG") {
            minLevel = LogLevel::DEBUG;
        }
        else if (arg == "INFO") {
            minLevel = LogLevel::INFO;
        }
        else if (arg == "WARNING" || arg == "WARN") {
            minLevel = LogLevel::WARNING;
        }
        else if (arg == "ERROR") {
            minLevel = LogLevel::ERROR;
        }
        else if (arg == "CRITICAL" || arg == "CRIT") {
            minLevel = LogLevel::CRITICAL;
        }
        else {
            sendResponse("Error: Unknown parameter '" + args[i] + "'");
            sendResponse("Usage: LOG [COUNT=n] [LEVEL=level] [ALL] [STATS] [CLEAR]");
            sendResponse("       LOG [DEBUG|INFO|WARNING|ERROR|CRITICAL]");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
    }
    
    // Handle clear request
    if (clearLog) {
        logger->clear();
        sendResponse("Log entries cleared");
        return TNCCommandResult::SUCCESS;
    }
    
    // Handle stats request
    if (showStats) {
        auto stats = logger->getStats();
        sendResponse("System Log Statistics");
        sendResponse("====================");
        sendResponse("Total messages: " + String(stats.totalMessages));
        sendResponse("Dropped messages: " + String(stats.droppedMessages));
        sendResponse("Current entries: " + String(stats.currentEntries));
        sendResponse("Max capacity: " + String(stats.maxEntries));
        sendResponse("System uptime: " + formatTime(stats.uptimeMs));
        sendResponse("Min log level: " + String(SystemLogger::levelToString(logger->getMinLevel())));
        return TNCCommandResult::SUCCESS;
    }
    
    // Get and display log entries
    String logOutput = logger->getFormattedLog(count, minLevel);
    
    // Split into lines and send each one
    int startIdx = 0;
    int newlineIdx = 0;
    
    while ((newlineIdx = logOutput.indexOf('\n', startIdx)) != -1) {
        String line = logOutput.substring(startIdx, newlineIdx);
        if (line.length() > 0) {
            sendResponse(line);
        }
        startIdx = newlineIdx + 1;
    }
    
    // Send any remaining content
    if (startIdx < logOutput.length()) {
        String line = logOutput.substring(startIdx);
        if (line.length() > 0) {
            sendResponse(line);
        }
    }
    
    return TNCCommandResult::SUCCESS;
}
