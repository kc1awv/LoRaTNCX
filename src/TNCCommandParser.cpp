/**
 * @file TNCCommandParser.cpp
 * @brief TAPR TNC-2 style command interface implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "TNCCommandParser.h"

TNCCommandParser::TNCCommandParser() :
    currentMode(TNC_MODE_COMMAND),
    cmdBufferIndex(0),
    commandReady(false),
    argCount(0),
    mycall("NOCALL"),
    txDelay(50),        // 500ms default (TAPR TNC-2 compatible)
    persistence(63),    // p=0.25 default
    slotTime(10),       // 100ms default
    fullDuplex(false),
    retry(10),          // Default retry count
    pacLen(128),        // Default packet length
    maxFrame(7),        // Default max frames
    respTime(3000),     // Default response time (ms)
    frack(3000),        // Default framing acknowledge time (ms)
    kissOnFlag(false)   // KISS mode flag (set by KISS ON, activated by RESTART)
{
}

bool TNCCommandParser::begin() {
    currentMode = TNC_MODE_COMMAND;
    clearCommand();
    
    delay(100);  // Brief pause for startup
    printBanner();
    sendPrompt();
    
    return true;
}

void TNCCommandParser::processInputStream(char c) {
    if (currentMode == TNC_MODE_KISS) {
        // In KISS mode, we shouldn't be processing command input
        return;
    }
    
    // Handle backspace
    if (c == '\b' || c == 0x7F) {
        if (cmdBufferIndex > 0) {
            cmdBufferIndex--;
            Serial.print("\b \b");  // Backspace, space, backspace
        }
        return;
    }
    
    // Echo character (except control characters)
    if (c >= ' ' && c <= '~') {
        Serial.print(c);
    }
    
    // Handle end of line
    if (c == '\r' || c == '\n') {
        Serial.println();  // Ensure we're on a new line
        
        if (cmdBufferIndex > 0) {
            cmdBuffer[cmdBufferIndex] = '\0';
            commandReady = true;
        } else {
            sendPrompt();  // Empty line, just show prompt
        }
        return;
    }
    
    // Add to buffer if printable and space available
    if (c >= ' ' && c <= '~' && cmdBufferIndex < TNC_CMD_BUFFER_SIZE - 1) {
        cmdBuffer[cmdBufferIndex++] = c;
    }
}

bool TNCCommandParser::hasCommand() {
    return commandReady;
}

bool TNCCommandParser::processCommand() {
    if (!commandReady) {
        return false;
    }
    
    parseCommand();
    processCmd();
    clearCommand();
    
    if (currentMode == TNC_MODE_COMMAND) {
        sendPrompt();
    }
    
    return true;
}

void TNCCommandParser::setMode(TNCMode mode) {
    if (mode == currentMode) return;
    
    TNCMode oldMode = currentMode;
    currentMode = mode;
    
    if (mode == TNC_MODE_KISS) {
        Serial.println("Entering KISS mode...");
        if (onKissModeEnter) {
            onKissModeEnter();
        }
    } else if (mode == TNC_MODE_COMMAND) {
        if (oldMode == TNC_MODE_KISS) {
            Serial.println();  // New line after KISS mode
            Serial.println("Exiting KISS mode...");
            if (onKissModeExit) {
                onKissModeExit();
            }
        }
        printBanner();
        sendPrompt();
    }
}

void TNCCommandParser::printBanner() {
    Serial.println();
    Serial.println("============================================");
    Serial.println("   LoRaTNCX v2.0 - LoRa TNC with KISS");
    Serial.println("   Compatible with TAPR TNC-2 commands");
    Serial.println("   Hardware: Heltec WiFi LoRa 32 V4");
    Serial.println("============================================");
    Serial.println();
    Serial.println("Type 'HELP' for command list");
    Serial.printf("Current callsign: %s\n", mycall.c_str());
    Serial.println();
}

void TNCCommandParser::printHelp() {
    Serial.println();
    Serial.println("Available Commands:");
    Serial.println("==================");
    Serial.println("KISS ON          - Set KISS flag (requires RESTART to activate)");
    Serial.println("KISS OFF         - Clear KISS flag");
    Serial.println("KISSM            - Immediately enter KISS mode");
    Serial.println("RESTART          - Restart TNC (enter KISS mode if KISS ON)");
    Serial.println();
    Serial.println("Configuration:");
    Serial.println("MYCALL <call>    - Set station callsign");
    Serial.println("TXDELAY <n>      - Set TX delay (10ms units, default 50)");
    Serial.println("PERSIST <n>      - Set persistence (0-255, default 63)");
    Serial.println("SLOTTIME <n>     - Set slot time (10ms units, default 10)");
    Serial.println("FULLDUPLEX <0|1> - Set duplex mode (0=half, 1=full)");
    Serial.println("RETRY <n>        - Set retry count (default 10)");
    Serial.println("PACLEN <n>       - Set packet length (default 128)");
    Serial.println("MAXFRAME <n>     - Set max frames (default 7)");
    Serial.println("RESPTIME <n>     - Set response time ms (default 3000)");
    Serial.println("FRACK <n>        - Set frame ack time ms (default 3000)");
    Serial.println();
    Serial.println("Information:");
    Serial.println("DISPLAY          - Show current parameters");
    Serial.println("STATUS           - Show system status");
    Serial.println("VERSION          - Show version information");
    Serial.println("BANNER           - Show welcome banner");
    Serial.println("HELP             - Show this help");
    Serial.println();
}

void TNCCommandParser::printStatus() {
    Serial.println();
    Serial.println("=== System Status ===");
    Serial.printf("Mode: %s\n", (currentMode == TNC_MODE_KISS) ? "KISS" : "Command");
    Serial.printf("Uptime: %lu ms\n", millis());
    Serial.printf("Free RAM: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.println("=====================");
    Serial.println();
}

void TNCCommandParser::parseCommand() {
    String cmdLine = String(cmdBuffer);
    cmdLine.trim();
    cmdLine.toUpperCase();
    
    // Split into command and arguments
    int spaceIndex = cmdLine.indexOf(' ');
    
    if (spaceIndex == -1) {
        command = cmdLine;
        argCount = 0;
    } else {
        command = cmdLine.substring(0, spaceIndex);
        String argsString = cmdLine.substring(spaceIndex + 1);
        argsString.trim();
        
        // Split arguments
        argCount = 0;
        int startIndex = 0;
        
        while (startIndex < argsString.length() && argCount < TNC_MAX_ARGS) {
            int nextSpace = argsString.indexOf(' ', startIndex);
            
            if (nextSpace == -1) {
                args[argCount++] = argsString.substring(startIndex);
                break;
            } else {
                String arg = argsString.substring(startIndex, nextSpace);
                if (arg.length() > 0) {
                    args[argCount++] = arg;
                }
                startIndex = nextSpace + 1;
            }
        }
    }
}

void TNCCommandParser::clearCommand() {
    cmdBufferIndex = 0;
    commandReady = false;
    command = "";
    argCount = 0;
    memset(cmdBuffer, 0, TNC_CMD_BUFFER_SIZE);
}

void TNCCommandParser::processCmd() {
    if (command.length() == 0) return;
    
    // Command dispatch
    if (command == "KISS") {
        handleKiss(argCount > 0 ? args[0] : "");
    } else if (command == "KISSM") {
        handleKissImmediate();
    } else if (command == "RESTART") {
        handleRestart();
    } else if (command == "MYCALL") {
        handleMycall(argCount > 0 ? args[0] : "");
    } else if (command == "TXDELAY") {
        handleTxDelay(argCount > 0 ? args[0] : "");
    } else if (command == "PERSIST") {
        handlePersist(argCount > 0 ? args[0] : "");
    } else if (command == "SLOTTIME") {
        handleSlotTime(argCount > 0 ? args[0] : "");
    } else if (command == "FULLDUPLEX") {
        handleFullDuplex(argCount > 0 ? args[0] : "");
    } else if (command == "RETRY") {
        handleRetry(argCount > 0 ? args[0] : "");
    } else if (command == "PACLEN") {
        handlePacLen(argCount > 0 ? args[0] : "");
    } else if (command == "MAXFRAME") {
        handleMaxFrame(argCount > 0 ? args[0] : "");
    } else if (command == "RESPTIME") {
        handleRespTime(argCount > 0 ? args[0] : "");
    } else if (command == "FRACK") {
        handleFrack(argCount > 0 ? args[0] : "");
    } else if (command == "DISPLAY") {
        handleDisplay();
    } else if (command == "STATUS") {
        handleStatus();
    } else if (command == "VERSION") {
        handleVersion();
    } else if (command == "BANNER") {
        handleBanner();
    } else if (command == "HELP" || command == "?") {
        handleHelp();
    } else {
        sendResponse("?Invalid command - type HELP for command list");
    }
}

void TNCCommandParser::handleKiss(const String& arg) {
    if (arg == "ON") {
        kissOnFlag = true;
        sendResponse("KISS ON - Issue RESTART command to activate KISS mode");
        sendResponse("To return to normal operation, send bytes $C0, $FF, $C0");
    } else if (arg == "OFF") {
        kissOnFlag = false;
        sendResponse("KISS OFF - Will boot into command mode on next RESTART");
    } else if (arg == "" || arg == "?") {
        sendResponse("KISS: " + String(kissOnFlag ? "ON" : "OFF"));
    } else {
        sendResponse("Usage: KISS ON|OFF");
    }
}

void TNCCommandParser::handleKissImmediate() {
    sendResponse("KISSM - Immediately entering KISS mode");
    sendResponse("Status LED will flash three times");
    delay(100);
    
    // Flash LEDs three times as per TAPR TNC-2 spec
    // (We'll implement this in the callback)
    
    // Enter KISS mode immediately
    setMode(TNC_MODE_KISS);
}

void TNCCommandParser::handleRestart() {
    if (kissOnFlag) {
        sendResponse("Restarting TNC into KISS mode...");
        delay(100);
        
        // Enter KISS mode after restart (LED flashing will be handled by TNCManager)
        setMode(TNC_MODE_KISS);
    } else {
        sendResponse("Restarting TNC into command mode...");
        delay(100);
        
        if (onRestart) {
            onRestart();
        } else {
            // Reset to command mode
            setMode(TNC_MODE_COMMAND);
        }
    }
}

void TNCCommandParser::handleMycall(const String& arg) {
    if (arg.length() > 0) {
        mycall = arg;
        sendResponse("MYCALL set to " + mycall);
    } else {
        sendResponse("MYCALL: " + mycall);
    }
}

void TNCCommandParser::handleTxDelay(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, txDelay);
        if (value >= 0 && value <= 255) {
            txDelay = value;
            sendResponse("TXDELAY: " + String(txDelay) + " (" + String(txDelay * 10) + "ms)");
        } else {
            sendResponse("TXDELAY must be 0-255");
        }
    } else {
        sendResponse("TXDELAY: " + String(txDelay) + " (" + String(txDelay * 10) + "ms)");
    }
}

void TNCCommandParser::handlePersist(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, persistence);
        if (value >= 0 && value <= 255) {
            persistence = value;
            float p = (persistence + 1) / 256.0;
            sendResponse("PERSIST: " + String(persistence) + " (p=" + String(p, 3) + ")");
        } else {
            sendResponse("PERSIST must be 0-255");
        }
    } else {
        float p = (persistence + 1) / 256.0;
        sendResponse("PERSIST: " + String(persistence) + " (p=" + String(p, 3) + ")");
    }
}

void TNCCommandParser::handleSlotTime(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, slotTime);
        if (value >= 0 && value <= 255) {
            slotTime = value;
            sendResponse("SLOTTIME: " + String(slotTime) + " (" + String(slotTime * 10) + "ms)");
        } else {
            sendResponse("SLOTTIME must be 0-255");
        }
    } else {
        sendResponse("SLOTTIME: " + String(slotTime) + " (" + String(slotTime * 10) + "ms)");
    }
}

void TNCCommandParser::handleFullDuplex(const String& arg) {
    if (arg.length() > 0) {
        bool value = parseBoolean(arg);
        fullDuplex = value;
        sendResponse("FULLDUPLEX: " + String(fullDuplex ? "ON" : "OFF"));
    } else {
        sendResponse("FULLDUPLEX: " + String(fullDuplex ? "ON" : "OFF"));
    }
}

void TNCCommandParser::handleRetry(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, retry);
        if (value >= 0 && value <= 255) {
            retry = value;
            sendResponse("RETRY: " + String(retry));
        } else {
            sendResponse("RETRY must be 0-255");
        }
    } else {
        sendResponse("RETRY: " + String(retry));
    }
}

void TNCCommandParser::handlePacLen(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, pacLen);
        if (value >= 1 && value <= 512) {
            pacLen = value;
            sendResponse("PACLEN: " + String(pacLen));
        } else {
            sendResponse("PACLEN must be 1-512");
        }
    } else {
        sendResponse("PACLEN: " + String(pacLen));
    }
}

void TNCCommandParser::handleMaxFrame(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, maxFrame);
        if (value >= 1 && value <= 7) {
            maxFrame = value;
            sendResponse("MAXFRAME: " + String(maxFrame));
        } else {
            sendResponse("MAXFRAME must be 1-7");
        }
    } else {
        sendResponse("MAXFRAME: " + String(maxFrame));
    }
}

void TNCCommandParser::handleRespTime(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, respTime);
        if (value >= 100 && value <= 30000) {
            respTime = value;
            sendResponse("RESPTIME: " + String(respTime) + "ms");
        } else {
            sendResponse("RESPTIME must be 100-30000ms");
        }
    } else {
        sendResponse("RESPTIME: " + String(respTime) + "ms");
    }
}

void TNCCommandParser::handleFrack(const String& arg) {
    if (arg.length() > 0) {
        int value = parseInteger(arg, frack);
        if (value >= 100 && value <= 30000) {
            frack = value;
            sendResponse("FRACK: " + String(frack) + "ms");
        } else {
            sendResponse("FRACK must be 100-30000ms");
        }
    } else {
        sendResponse("FRACK: " + String(frack) + "ms");
    }
}

void TNCCommandParser::handleDisplay() {
    Serial.println();
    Serial.println("=== TNC Parameters ===");
    Serial.printf("MYCALL:     %s\n", mycall.c_str());
    Serial.printf("TXDELAY:    %d (%dms)\n", txDelay, txDelay * 10);
    Serial.printf("PERSIST:    %d (p=%.3f)\n", persistence, (persistence + 1) / 256.0);
    Serial.printf("SLOTTIME:   %d (%dms)\n", slotTime, slotTime * 10);
    Serial.printf("FULLDUPLEX: %s\n", fullDuplex ? "ON" : "OFF");
    Serial.printf("KISS:       %s\n", kissOnFlag ? "ON" : "OFF");
    Serial.printf("RETRY:      %d\n", retry);
    Serial.printf("PACLEN:     %d\n", pacLen);
    Serial.printf("MAXFRAME:   %d\n", maxFrame);
    Serial.printf("RESPTIME:   %dms\n", respTime);
    Serial.printf("FRACK:      %dms\n", frack);
    Serial.println("======================");
    Serial.println();
}

void TNCCommandParser::handleStatus() {
    printStatus();
}

void TNCCommandParser::handleVersion() {
    Serial.println();
    Serial.println("LoRaTNCX v2.0");
    Serial.println("LoRa TNC with KISS Protocol Support");
    Serial.println("Hardware: Heltec WiFi LoRa 32 V4 (ESP32-S3)");
    Serial.println("LoRa Chip: SX1262");
    Serial.println("RadioLib: v7.4.0");
    Serial.println("Compatible with TAPR TNC-2 commands");
    Serial.println("Built: " __DATE__ " " __TIME__);
    Serial.println();
}

void TNCCommandParser::handleBanner() {
    printBanner();
}

void TNCCommandParser::handleHelp() {
    printHelp();
}

void TNCCommandParser::sendResponse(const String& response) {
    Serial.println(response);
}

void TNCCommandParser::sendPrompt() {
    Serial.print("cmd:");
}

int TNCCommandParser::parseInteger(const String& str, int defaultValue) {
    if (str.length() == 0) return defaultValue;
    
    // Simple integer parsing
    int value = str.toInt();
    return value;
}

bool TNCCommandParser::parseBoolean(const String& str) {
    if (str == "1" || str == "ON" || str == "TRUE" || str == "YES") {
        return true;
    }
    return false;
}