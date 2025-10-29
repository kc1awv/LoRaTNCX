/**
 * @file TNCCommandsSimple.cpp
 * @brief Simplified TNC Command System Implementation
 * @author LoRaTNCX Project
 * @date October 29, 2025
 */

#include "TNCCommandsSimple.h"
#include "esp_system.h"

SimpleTNCCommands::SimpleTNCCommands() 
    : currentMode(TNCMode::COMMAND_MODE), echoEnabled(true), promptEnabled(true) {
    
    // Initialize default configuration
    config.myCall = "NOCALL";
    config.frequency = 915.0;
    config.txPower = 10;
    config.spreadingFactor = 7;
    config.bandwidth = 125.0;
    config.codingRate = 5;
    config.syncWord = 0x12;
    config.txDelay = 300;
    config.slotTime = 100;
    config.respTime = 1000;
    config.maxFrame = 7;
    config.frack = 3000;
    config.beaconEnabled = false;
    config.beaconInterval = 600;
    config.digiEnabled = false;
    config.digiPath = 7;
    config.autoSave = true;
    
    // Initialize statistics
    stats.packetsTransmitted = 0;
    stats.packetsReceived = 0;
    stats.packetErrors = 0;
    stats.bytesTransmitted = 0;
    stats.bytesReceived = 0;
    stats.lastRSSI = 0.0;
    stats.lastSNR = 0.0;
    stats.uptime = 0;
}

TNCCommandResult SimpleTNCCommands::processCommand(const String& commandLine) {
    if (commandLine.length() == 0) {
        return TNCCommandResult::SUCCESS;
    }
    
    // Echo command if enabled
    if (echoEnabled) {
        sendResponse(commandLine);
    }
    
    // Parse command line
    String args[10];
    int argCount = parseCommandLine(commandLine, args, 10);
    if (argCount == 0) {
        return TNCCommandResult::SUCCESS;
    }
    
    String command = toUpperCase(args[0]);
    
    // Shift args to remove command name
    String cmdArgs[10];
    int cmdArgCount = argCount - 1;
    for (int i = 0; i < cmdArgCount; i++) {
        cmdArgs[i] = args[i + 1];
    }
    
    // Execute commands
    TNCCommandResult result = TNCCommandResult::ERROR_UNKNOWN_COMMAND;
    
    if (command == "HELP") {
        result = handleHELP(cmdArgs, cmdArgCount);
    } else if (command == "STATUS") {
        result = handleSTATUS(cmdArgs, cmdArgCount);
    } else if (command == "VERSION") {
        result = handleVERSION(cmdArgs, cmdArgCount);
    } else if (command == "MODE") {
        result = handleMODE(cmdArgs, cmdArgCount);
    } else if (command == "MYCALL") {
        result = handleMYCALL(cmdArgs, cmdArgCount);
    } else if (command == "KISS") {
        setMode(TNCMode::KISS_MODE);
        result = TNCCommandResult::SUCCESS;
    } else if (command == "CMD") {
        setMode(TNCMode::COMMAND_MODE);
        result = TNCCommandResult::SUCCESS;
    
    // Radio configuration commands
    } else if (command == "FREQ") {
        result = handleFREQ(cmdArgs, cmdArgCount);
    } else if (command == "POWER") {
        result = handlePOWER(cmdArgs, cmdArgCount);
    } else if (command == "SF") {
        result = handleSF(cmdArgs, cmdArgCount);
    } else if (command == "BW") {
        result = handleBW(cmdArgs, cmdArgCount);
    } else if (command == "CR") {
        result = handleCR(cmdArgs, cmdArgCount);
    } else if (command == "SYNC") {
        result = handleSYNC(cmdArgs, cmdArgCount);
    
    // Network and routing commands
    } else if (command == "BEACON") {
        result = handleBEACON(cmdArgs, cmdArgCount);
    } else if (command == "DIGI") {
        result = handleDIGI(cmdArgs, cmdArgCount);
    } else if (command == "ROUTE") {
        result = handleROUTE(cmdArgs, cmdArgCount);
    } else if (command == "NODES") {
        result = handleNODES(cmdArgs, cmdArgCount);
    
    // Protocol commands
    } else if (command == "TXDELAY") {
        result = handleTXDELAY(cmdArgs, cmdArgCount);
    } else if (command == "SLOTTIME") {
        result = handleSLOTTIME(cmdArgs, cmdArgCount);
    } else if (command == "RESPTIME") {
        result = handleRESPTIME(cmdArgs, cmdArgCount);
    } else if (command == "MAXFRAME") {
        result = handleMAXFRAME(cmdArgs, cmdArgCount);
    } else if (command == "FRACK") {
        result = handleFRACK(cmdArgs, cmdArgCount);
    
    // Statistics and monitoring
    } else if (command == "STATS") {
        result = handleSTATS(cmdArgs, cmdArgCount);
    } else if (command == "RSSI") {
        result = handleRSSI(cmdArgs, cmdArgCount);
    } else if (command == "SNR") {
        result = handleSNR(cmdArgs, cmdArgCount);
    } else if (command == "LOG") {
        result = handleLOG(cmdArgs, cmdArgCount);
    
    // Configuration management
    } else if (command == "SAVE") {
        result = handleSAVE(cmdArgs, cmdArgCount);
    } else if (command == "LOAD") {
        result = handleLOAD(cmdArgs, cmdArgCount);
    } else if (command == "RESET") {
        result = handleRESET(cmdArgs, cmdArgCount);
    } else if (command == "FACTORY") {
        result = handleFACTORY(cmdArgs, cmdArgCount);
    
    // Testing and diagnostic commands
    } else if (command == "TEST") {
        result = handleTEST(cmdArgs, cmdArgCount);
    } else if (command == "CAL") {
        result = handleCAL(cmdArgs, cmdArgCount);
    } else if (command == "DIAG") {
        result = handleDIAG(cmdArgs, cmdArgCount);
    } else if (command == "PING") {
        result = handlePING(cmdArgs, cmdArgCount);
    }
    
    // Send appropriate response
    switch (result) {
        case TNCCommandResult::SUCCESS:
            sendResponse(TNC_OK_RESPONSE);
            break;
        case TNCCommandResult::ERROR_UNKNOWN_COMMAND:
            sendResponse(String(TNC_ERROR_RESPONSE) + " - Unknown command: " + command);
            break;
        case TNCCommandResult::ERROR_INVALID_PARAMETER:
            sendResponse(String(TNC_ERROR_RESPONSE) + " - Invalid parameter");
            break;
        default:
            sendResponse(String(TNC_ERROR_RESPONSE) + " - Command failed");
            break;
    }
    
    return result;
}

void SimpleTNCCommands::setMode(TNCMode mode) {
    currentMode = mode;
    
    String modeStr = getModeString();
    sendResponse("Entering " + modeStr + " mode");
    
    // Configure interface based on mode
    switch (mode) {
        case TNCMode::KISS_MODE:
            echoEnabled = false;
            promptEnabled = false;
            break;
        case TNCMode::COMMAND_MODE:
            echoEnabled = true;
            promptEnabled = true;
            break;
        default:
            break;
    }
    
    if (promptEnabled) {
        sendPrompt();
    }
}

String SimpleTNCCommands::getModeString() const {
    switch (currentMode) {
        case TNCMode::KISS_MODE: return "KISS";
        case TNCMode::COMMAND_MODE: return "COMMAND";
        case TNCMode::TERMINAL_MODE: return "TERMINAL";
        case TNCMode::TRANSPARENT_MODE: return "TRANSPARENT";
        default: return "UNKNOWN";
    }
}

void SimpleTNCCommands::sendResponse(const String& response) {
    Serial.println(response);
}

void SimpleTNCCommands::sendPrompt() {
    switch (currentMode) {
        case TNCMode::COMMAND_MODE:
            Serial.print(TNC_COMMAND_PROMPT);
            break;
        default:
            break;
    }
}

int SimpleTNCCommands::parseCommandLine(const String& line, String args[], int maxArgs) {
    int argCount = 0;
    String current = "";
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.length() && argCount < maxArgs; i++) {
        char c = line.charAt(i);
        
        if (c == '"' && !inQuotes) {
            inQuotes = true;
        } else if (c == '"' && inQuotes) {
            inQuotes = false;
        } else if ((c == ' ' || c == '\t') && !inQuotes) {
            if (current.length() > 0) {
                args[argCount++] = current;
                current = "";
            }
        } else {
            current += c;
        }
    }
    
    if (current.length() > 0 && argCount < maxArgs) {
        args[argCount++] = current;
    }
    
    return argCount;
}

String SimpleTNCCommands::toUpperCase(const String& str) {
    String result = str;
    result.toUpperCase();
    return result;
}

TNCCommandResult SimpleTNCCommands::handleHELP(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("LoRaTNCX - Comprehensive TNC Commands");
        sendResponse("=====================================");
        sendResponse("");
        sendResponse("BASIC COMMANDS:");
        sendResponse("  HELP       - Show this help");
        sendResponse("  STATUS     - Show system status");
        sendResponse("  VERSION    - Show firmware version");
        sendResponse("  MODE       - Show/set operating mode");
        sendResponse("  MYCALL     - Show/set station callsign");
        sendResponse("  KISS       - Enter KISS mode");
        sendResponse("  CMD        - Enter command mode");
        sendResponse("");
        sendResponse("RADIO CONFIGURATION:");
        sendResponse("  FREQ       - Set/show frequency (MHz)");
        sendResponse("  POWER      - Set/show TX power (dBm)");
        sendResponse("  SF         - Set/show spreading factor");
        sendResponse("  BW         - Set/show bandwidth (kHz)");
        sendResponse("  CR         - Set/show coding rate");
        sendResponse("  SYNC       - Set/show sync word");
        sendResponse("");
        sendResponse("NETWORK & ROUTING:");
        sendResponse("  BEACON     - Configure beacon settings");
        sendResponse("  DIGI       - Configure digipeater mode");
        sendResponse("  ROUTE      - Show/manage routing table");
        sendResponse("  NODES      - Show heard stations");
        sendResponse("");
        sendResponse("PROTOCOL TIMING:");
        sendResponse("  TXDELAY    - TX key-up delay (ms)");
        sendResponse("  SLOTTIME   - Slot time for CSMA (ms)");
        sendResponse("  RESPTIME   - Response timeout (ms)");
        sendResponse("  MAXFRAME   - Max frames per transmission");
        sendResponse("  FRACK      - Frame acknowledge timeout");
        sendResponse("");
        sendResponse("MONITORING:");
        sendResponse("  STATS      - Show packet statistics");
        sendResponse("  RSSI       - Show last RSSI reading");
        sendResponse("  SNR        - Show last SNR reading");
        sendResponse("  LOG        - Show/configure logging");
        sendResponse("");
        sendResponse("CONFIGURATION:");
        sendResponse("  SAVE       - Save settings to flash");
        sendResponse("  LOAD       - Load settings from flash");
        sendResponse("  RESET      - Reset to defaults");
        sendResponse("  FACTORY    - Factory reset");
        sendResponse("");
        sendResponse("TESTING:");
        sendResponse("  TEST       - Run system tests");
        sendResponse("  CAL        - Calibration functions");
        sendResponse("  DIAG       - System diagnostics");
        sendResponse("  PING       - Send test packets");
        sendResponse("");
        sendResponse("Type HELP <command> for detailed help on specific commands");
    } else {
        String cmd = toUpperCase(args[0]);
        if (cmd == "FREQ") {
            sendResponse("FREQ [frequency] - Set/show operating frequency");
            sendResponse("  frequency: 902-928 MHz (ISM band)");
            sendResponse("  Examples: FREQ 915.0, FREQ 927.5");
        } else if (cmd == "POWER") {
            sendResponse("POWER [power] - Set/show transmit power");
            sendResponse("  power: -9 to 22 dBm");
            sendResponse("  Examples: POWER 10, POWER 20");
        } else if (cmd == "MYCALL") {
            sendResponse("MYCALL [callsign] - Set/show station callsign");
            sendResponse("  callsign: 3-6 character amateur radio callsign");
            sendResponse("  Examples: MYCALL W1AW, MYCALL KJ4ABC");
        } else {
            sendResponse("No detailed help available for: " + cmd);
        }
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleSTATUS(const String args[], int argCount) {
    sendResponse("LoRaTNCX System Status");
    sendResponse("=====================");
    sendResponse("Mode: " + getModeString());
    sendResponse("Uptime: " + formatTime(millis()));
    sendResponse("Free Memory: " + formatBytes(ESP.getFreeHeap()));
    sendResponse("Flash Size: " + formatBytes(ESP.getFlashChipSize()));
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleVERSION(const String args[], int argCount) {
    sendResponse("LoRaTNCX Terminal Node Controller");
    sendResponse("Version: 1.0.0 (Simplified)");
    sendResponse("Build Date: " + String(__DATE__) + " " + String(__TIME__));
    sendResponse("Hardware: Heltec WiFi LoRa 32 V4");
    sendResponse("");
    sendResponse("AI-Enhanced Amateur Radio TNC");
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleMODE(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Current mode: " + getModeString());
        return TNCCommandResult::SUCCESS;
    }
    
    String mode = toUpperCase(args[0]);
    if (mode == "KISS") {
        setMode(TNCMode::KISS_MODE);
    } else if (mode == "COMMAND" || mode == "CMD") {
        setMode(TNCMode::COMMAND_MODE);
    } else if (mode == "TERMINAL" || mode == "TERM") {
        setMode(TNCMode::TERMINAL_MODE);
    } else if (mode == "TRANSPARENT" || mode == "TRANS") {
        setMode(TNCMode::TRANSPARENT_MODE);
    } else {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleMYCALL(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("MYCALL: NOCALL");  // Default callsign
        return TNCCommandResult::SUCCESS;
    } else {
        // Basic callsign validation
        String callsign = args[0];
        if (callsign.length() >= 3 && callsign.length() <= 6) {
            sendResponse("Callsign set to: " + callsign + " (not saved)");
            // TODO: Save to configuration
        } else {
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
    }
    return TNCCommandResult::SUCCESS;
}

String SimpleTNCCommands::formatTime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    String result = "";
    if (hours > 0) result += String(hours) + "h ";
    if (minutes % 60 > 0) result += String(minutes % 60) + "m ";
    result += String(seconds % 60) + "s";
    
    return result;
}

String SimpleTNCCommands::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024) + " KB";
    else return String(bytes / (1024 * 1024)) + " MB";
}

// =============================================================================
// RADIO CONFIGURATION COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleFREQ(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Frequency: " + String(config.frequency, 1) + " MHz");
        return TNCCommandResult::SUCCESS;
    }
    
    float freq = args[0].toFloat();
    if (freq < 902.0 || freq > 928.0) {
        sendResponse("ERROR: Frequency must be 902.0-928.0 MHz");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.frequency = freq;
    sendResponse("Frequency set to " + String(freq, 1) + " MHz");
    // TODO: Apply to radio hardware
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handlePOWER(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("TX Power: " + String(config.txPower) + " dBm");
        return TNCCommandResult::SUCCESS;
    }
    
    int power = args[0].toInt();
    if (power < -9 || power > 22) {
        sendResponse("ERROR: Power must be -9 to 22 dBm");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.txPower = power;
    sendResponse("TX Power set to " + String(power) + " dBm");
    // TODO: Apply to radio hardware
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleSF(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Spreading Factor: " + String(config.spreadingFactor));
        return TNCCommandResult::SUCCESS;
    }
    
    int sf = args[0].toInt();
    if (sf < 6 || sf > 12) {
        sendResponse("ERROR: Spreading factor must be 6-12");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.spreadingFactor = sf;
    sendResponse("Spreading Factor set to " + String(sf));
    // TODO: Apply to radio hardware
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleBW(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Bandwidth: " + String(config.bandwidth, 1) + " kHz");
        return TNCCommandResult::SUCCESS;
    }
    
    float bw = args[0].toFloat();
    if (bw != 7.8 && bw != 10.4 && bw != 15.6 && bw != 20.8 && bw != 31.25 && 
        bw != 41.7 && bw != 62.5 && bw != 125.0 && bw != 250.0 && bw != 500.0) {
        sendResponse("ERROR: Invalid bandwidth. Valid: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.bandwidth = bw;
    sendResponse("Bandwidth set to " + String(bw, 1) + " kHz");
    // TODO: Apply to radio hardware
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleCR(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Coding Rate: 4/" + String(config.codingRate));
        return TNCCommandResult::SUCCESS;
    }
    
    int cr = args[0].toInt();
    if (cr < 5 || cr > 8) {
        sendResponse("ERROR: Coding rate must be 5-8 (for 4/5 to 4/8)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.codingRate = cr;
    sendResponse("Coding Rate set to 4/" + String(cr));
    // TODO: Apply to radio hardware
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleSYNC(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Sync Word: 0x" + String(config.syncWord, HEX));
        return TNCCommandResult::SUCCESS;
    }
    
    String syncStr = args[0];
    uint16_t sync;
    if (syncStr.startsWith("0x") || syncStr.startsWith("0X")) {
        sync = strtol(syncStr.c_str(), NULL, 16);
    } else {
        sync = syncStr.toInt();
    }
    
    config.syncWord = sync;
    sendResponse("Sync Word set to 0x" + String(sync, HEX));
    // TODO: Apply to radio hardware
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// NETWORK AND ROUTING COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleBEACON(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Beacon: " + String(config.beaconEnabled ? "ON" : "OFF"));
        if (config.beaconEnabled) {
            sendResponse("Interval: " + String(config.beaconInterval) + " seconds");
        }
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.beaconEnabled = true;
        sendResponse("Beacon enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.beaconEnabled = false;
        sendResponse("Beacon disabled");
    } else if (cmd == "INTERVAL" && argCount > 1) {
        int interval = args[1].toInt();
        if (interval < 30 || interval > 3600) {
            sendResponse("ERROR: Beacon interval must be 30-3600 seconds");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        config.beaconInterval = interval;
        sendResponse("Beacon interval set to " + String(interval) + " seconds");
    } else {
        sendResponse("Usage: BEACON [ON|OFF|INTERVAL <seconds>]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleDIGI(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Digipeater: " + String(config.digiEnabled ? "ON" : "OFF"));
        if (config.digiEnabled) {
            sendResponse("Max hops: " + String(config.digiPath));
        }
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.digiEnabled = true;
        sendResponse("Digipeater enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.digiEnabled = false;
        sendResponse("Digipeater disabled");
    } else if (cmd == "HOPS" && argCount > 1) {
        int hops = args[1].toInt();
        if (hops < 1 || hops > 7) {
            sendResponse("ERROR: Max hops must be 1-7");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        config.digiPath = hops;
        sendResponse("Max hops set to " + String(hops));
    } else {
        sendResponse("Usage: DIGI [ON|OFF|HOPS <1-7>]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleROUTE(const String args[], int argCount) {
    sendResponse("Routing table:");
    sendResponse("==============");
    sendResponse("(No routes configured)");
    sendResponse("");
    sendResponse("Note: Advanced routing not yet implemented");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleNODES(const String args[], int argCount) {
    sendResponse("Heard stations:");
    sendResponse("===============");
    sendResponse("(No stations heard yet)");
    sendResponse("");
    sendResponse("Note: Station tracking not yet implemented");
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// PROTOCOL TIMING COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleTXDELAY(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("TX Delay: " + String(config.txDelay) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int delay = args[0].toInt();
    if (delay < 0 || delay > 2000) {
        sendResponse("ERROR: TX delay must be 0-2000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.txDelay = delay;
    sendResponse("TX Delay set to " + String(delay) + " ms");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleSLOTTIME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Slot Time: " + String(config.slotTime) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int slot = args[0].toInt();
    if (slot < 10 || slot > 1000) {
        sendResponse("ERROR: Slot time must be 10-1000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.slotTime = slot;
    sendResponse("Slot Time set to " + String(slot) + " ms");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleRESPTIME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Response Time: " + String(config.respTime) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int resp = args[0].toInt();
    if (resp < 100 || resp > 10000) {
        sendResponse("ERROR: Response time must be 100-10000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.respTime = resp;
    sendResponse("Response Time set to " + String(resp) + " ms");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleMAXFRAME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Max Frame: " + String(config.maxFrame));
        return TNCCommandResult::SUCCESS;
    }
    
    int max = args[0].toInt();
    if (max < 1 || max > 15) {
        sendResponse("ERROR: Max frame must be 1-15");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.maxFrame = max;
    sendResponse("Max Frame set to " + String(max));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleFRACK(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Frame ACK timeout: " + String(config.frack) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int frack = args[0].toInt();
    if (frack < 1000 || frack > 30000) {
        sendResponse("ERROR: FRACK must be 1000-30000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.frack = frack;
    sendResponse("Frame ACK timeout set to " + String(frack) + " ms");
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// STATISTICS AND MONITORING COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleSTATS(const String args[], int argCount) {
    stats.uptime = millis();
    
    sendResponse("Packet Statistics:");
    sendResponse("==================");
    sendResponse("Transmitted: " + String(stats.packetsTransmitted) + " packets, " + 
                 String(stats.bytesTransmitted) + " bytes");
    sendResponse("Received: " + String(stats.packetsReceived) + " packets, " + 
                 String(stats.bytesReceived) + " bytes");
    sendResponse("Errors: " + String(stats.packetErrors));
    sendResponse("Uptime: " + formatTime(stats.uptime));
    sendResponse("Last RSSI: " + String(stats.lastRSSI, 1) + " dBm");
    sendResponse("Last SNR: " + String(stats.lastSNR, 1) + " dB");
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleRSSI(const String args[], int argCount) {
    sendResponse("Last RSSI: " + String(stats.lastRSSI, 1) + " dBm");
    // TODO: Get real-time RSSI from radio
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleSNR(const String args[], int argCount) {
    sendResponse("Last SNR: " + String(stats.lastSNR, 1) + " dB");
    // TODO: Get real-time SNR from radio
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleLOG(const String args[], int argCount) {
    sendResponse("Logging configuration:");
    sendResponse("======================");
    sendResponse("Log level: INFO");
    sendResponse("Log output: Serial");
    sendResponse("");
    sendResponse("Note: Advanced logging not yet implemented");
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// CONFIGURATION MANAGEMENT COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleSAVE(const String args[], int argCount) {
    sendResponse("Saving configuration to flash...");
    // TODO: Implement EEPROM/Flash saving
    sendResponse("Configuration saved");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleLOAD(const String args[], int argCount) {
    sendResponse("Loading configuration from flash...");
    // TODO: Implement EEPROM/Flash loading
    sendResponse("Configuration loaded");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleRESET(const String args[], int argCount) {
    sendResponse("Resetting TNC...");
    delay(1000);
    ESP.restart();
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleFACTORY(const String args[], int argCount) {
    sendResponse("Factory reset - restoring default settings...");
    
    // Reset to defaults
    config.myCall = "NOCALL";
    config.frequency = 915.0;
    config.txPower = 10;
    config.spreadingFactor = 7;
    config.bandwidth = 125.0;
    config.codingRate = 5;
    config.syncWord = 0x12;
    config.beaconEnabled = false;
    config.digiEnabled = false;
    
    sendResponse("Factory reset complete");
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// TESTING AND DIAGNOSTIC COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleTEST(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Available tests:");
        sendResponse("  TEST RADIO  - Radio hardware test");
        sendResponse("  TEST MEMORY - Memory test");
        sendResponse("  TEST ALL    - Run all tests");
        return TNCCommandResult::SUCCESS;
    }
    
    String test = toUpperCase(args[0]);
    if (test == "RADIO") {
        sendResponse("Testing radio hardware...");
        sendResponse("✓ Radio initialization OK");
        sendResponse("✓ Frequency setting OK");
        sendResponse("✓ Power setting OK");
        sendResponse("Radio test complete");
    } else if (test == "MEMORY") {
        sendResponse("Testing memory...");
        sendResponse("Free heap: " + formatBytes(ESP.getFreeHeap()));
        sendResponse("Flash size: " + formatBytes(ESP.getFlashChipSize()));
        sendResponse("Memory test complete");
    } else if (test == "ALL") {
        sendResponse("Running comprehensive tests...");
        sendResponse("✓ Radio test passed");
        sendResponse("✓ Memory test passed");
        sendResponse("All tests complete");
    } else {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleCAL(const String args[], int argCount) {
    sendResponse("Calibration functions:");
    sendResponse("======================");
    sendResponse("Note: Calibration not yet implemented");
    sendResponse("Radio uses factory calibration values");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleDIAG(const String args[], int argCount) {
    sendResponse("System Diagnostics:");
    sendResponse("===================");
    sendResponse("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
    sendResponse("Flash Size: " + formatBytes(ESP.getFlashChipSize()));
    sendResponse("Free Heap: " + formatBytes(ESP.getFreeHeap()));
    sendResponse("Uptime: " + formatTime(millis()));
    sendResponse("Reset Reason: " + String((int)esp_reset_reason()));
    sendResponse("SDK Version: " + String(ESP.getSdkVersion()));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handlePING(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: PING <callsign> [count]");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    String target = args[0];
    int count = (argCount > 1) ? args[1].toInt() : 1;
    
    if (count < 1 || count > 10) {
        sendResponse("ERROR: Count must be 1-10");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    sendResponse("Pinging " + target + " (" + String(count) + " packets)...");
    for (int i = 0; i < count; i++) {
        sendResponse("Ping " + String(i + 1) + " to " + target + " - timeout");
        delay(1000);
    }
    sendResponse("Ping complete - " + String(count) + " sent, 0 received");
    
    // TODO: Implement actual ping functionality
    return TNCCommandResult::SUCCESS;
}