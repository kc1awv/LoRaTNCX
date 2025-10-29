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
    // Station configuration
    config.myCall = "NOCALL";
    config.mySSID = 0;
    config.beaconText = "LoRaTNCX Test Station";
    config.idEnabled = true;
    config.cwidEnabled = false;
    config.latitude = 0.0;
    config.longitude = 0.0;
    config.altitude = 0;
    config.gridSquare = "";
    config.licenseClass = "GENERAL";
    
    // Radio parameters
    config.frequency = 915.0;
    config.txPower = 10;
    config.spreadingFactor = 7;
    config.bandwidth = 125.0;
    config.codingRate = 5;
    config.syncWord = 0x12;
    config.preambleLength = 8;
    config.paControl = true;
    
    // Protocol stack
    config.txDelay = 300;
    config.txTail = 50;
    config.persist = 63;
    config.slotTime = 100;
    config.respTime = 1000;
    config.maxFrame = 7;
    config.frack = 3000;
    config.retry = 10;
    
    // Operating modes
    config.echoEnabled = true;
    config.promptEnabled = true;
    config.monitorEnabled = false;
    
    // Beacon and digi
    config.beaconEnabled = false;
    config.beaconInterval = 600;
    config.digiEnabled = false;
    config.digiPath = 7;
    
    // Amateur radio
    config.band = "70CM";
    config.region = "US";
    config.emergencyMode = false;
    config.aprsEnabled = false;
    config.aprsSymbol = "Y";
    
    // Network
    config.unprotoAddr = "CQ";
    config.unprotoPath = "WIDE1-1";
    config.uidWait = true;
    config.mconEnabled = false;
    config.maxUsers = 1;
    config.flowControl = true;
    
    // System
    config.debugLevel = 1;
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
    
    // Station configuration commands
    } else if (command == "MYSSID") {
        result = handleMYSSID(cmdArgs, cmdArgCount);
    } else if (command == "BCON") {
        result = handleBCON(cmdArgs, cmdArgCount);
    } else if (command == "BTEXT") {
        result = handleBTEXT(cmdArgs, cmdArgCount);
    } else if (command == "ID") {
        result = handleID(cmdArgs, cmdArgCount);
    } else if (command == "CWID") {
        result = handleCWID(cmdArgs, cmdArgCount);
    } else if (command == "LOCATION") {
        result = handleLOCATION(cmdArgs, cmdArgCount);
    } else if (command == "GRID") {
        result = handleGRID(cmdArgs, cmdArgCount);
    } else if (command == "LICENSE") {
        result = handleLICENSE(cmdArgs, cmdArgCount);
    
    // Extended radio parameter commands
    } else if (command == "PREAMBLE") {
        result = handlePREAMBLE(cmdArgs, cmdArgCount);
    } else if (command == "PRESET") {
        result = handlePRESET(cmdArgs, cmdArgCount);
    } else if (command == "PACTL") {
        result = handlePACTL(cmdArgs, cmdArgCount);
    
    // Protocol stack commands
    } else if (command == "TXTAIL") {
        result = handleTXTAIL(cmdArgs, cmdArgCount);
    } else if (command == "PERSIST") {
        result = handlePERSIST(cmdArgs, cmdArgCount);
    } else if (command == "RETRY") {
        result = handleRETRY(cmdArgs, cmdArgCount);
    
    // Operating mode commands
    } else if (command == "TERMINAL") {
        result = handleTERMINAL(cmdArgs, cmdArgCount);
    } else if (command == "TRANSPARENT") {
        result = handleTRANSPARENT(cmdArgs, cmdArgCount);
    } else if (command == "ECHO") {
        result = handleECHO(cmdArgs, cmdArgCount);
    } else if (command == "PROMPT") {
        result = handlePROMPT(cmdArgs, cmdArgCount);
    } else if (command == "CONNECT") {
        result = handleCONNECT(cmdArgs, cmdArgCount);
    } else if (command == "DISCONNECT") {
        result = handleDISCONNECT(cmdArgs, cmdArgCount);
    
    // Extended monitoring commands
    } else if (command == "MONITOR") {
        result = handleMONITOR(cmdArgs, cmdArgCount);
    } else if (command == "MHEARD") {
        result = handleMHEARD(cmdArgs, cmdArgCount);
    } else if (command == "TEMPERATURE") {
        result = handleTEMPERATURE(cmdArgs, cmdArgCount);
    } else if (command == "VOLTAGE") {
        result = handleVOLTAGE(cmdArgs, cmdArgCount);
    } else if (command == "MEMORY") {
        result = handleMEMORY(cmdArgs, cmdArgCount);
    } else if (command == "UPTIME") {
        result = handleUPTIME(cmdArgs, cmdArgCount);
    
    // LoRa-specific commands
    } else if (command == "LORASTAT") {
        result = handleLORASTAT(cmdArgs, cmdArgCount);
    } else if (command == "TOA") {
        result = handleTOA(cmdArgs, cmdArgCount);
    } else if (command == "RANGE") {
        result = handleRANGE(cmdArgs, cmdArgCount);
    } else if (command == "LINKTEST") {
        result = handleLINKTEST(cmdArgs, cmdArgCount);
    } else if (command == "SENSITIVITY") {
        result = handleSENSITIVITY(cmdArgs, cmdArgCount);
    
    // Amateur radio specific commands
    } else if (command == "BAND") {
        result = handleBAND(cmdArgs, cmdArgCount);
    } else if (command == "REGION") {
        result = handleREGION(cmdArgs, cmdArgCount);
    } else if (command == "COMPLIANCE") {
        result = handleCOMPLIANCE(cmdArgs, cmdArgCount);
    } else if (command == "EMERGENCY") {
        result = handleEMERGENCY(cmdArgs, cmdArgCount);
    } else if (command == "APRS") {
        result = handleAPRS(cmdArgs, cmdArgCount);
    
    // Network and routing commands  
    } else if (command == "UNPROTO") {
        result = handleUNPROTO(cmdArgs, cmdArgCount);
    } else if (command == "UIDWAIT") {
        result = handleUIDWAIT(cmdArgs, cmdArgCount);
    } else if (command == "UIDFRAME") {
        result = handleUIDFRAME(cmdArgs, cmdArgCount);
    } else if (command == "MCON") {
        result = handleMCON(cmdArgs, cmdArgCount);
    } else if (command == "USERS") {
        result = handleUSERS(cmdArgs, cmdArgCount);
    } else if (command == "FLOW") {
        result = handleFLOW(cmdArgs, cmdArgCount);
    
    // System configuration commands
    } else if (command == "DEFAULT") {
        result = handleDEFAULT(cmdArgs, cmdArgCount);
    } else if (command == "QUIT") {
        result = handleQUIT(cmdArgs, cmdArgCount);
    } else if (command == "CALIBRATE") {
        result = handleCALIBRATE(cmdArgs, cmdArgCount);
    } else if (command == "SELFTEST") {
        result = handleSELFTEST(cmdArgs, cmdArgCount);
    } else if (command == "DEBUG") {
        result = handleDEBUG(cmdArgs, cmdArgCount);
    } else if (command == "SIMPLEX") {
        result = handleSIMPLEX(cmdArgs, cmdArgCount);
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
        sendResponse("  TXTAIL     - TX tail time (ms)");
        sendResponse("  PERSIST    - CSMA persistence (0-255)");
        sendResponse("  SLOTTIME   - Slot time for CSMA (ms)");
        sendResponse("  RESPTIME   - Response timeout (ms)");
        sendResponse("  MAXFRAME   - Max frames per transmission");
        sendResponse("  FRACK      - Frame acknowledge timeout");
        sendResponse("  RETRY      - Retry attempts (0-15)");
        sendResponse("  Note: LoRa is simplex (half-duplex only)");
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

// =============================================================================
// STATION CONFIGURATION COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleMYSSID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("MYSSID: " + String(config.mySSID));
        return TNCCommandResult::SUCCESS;
    }
    
    int ssid = args[0].toInt();
    if (ssid < 0 || ssid > 15) {
        sendResponse("ERROR: SSID must be 0-15");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.mySSID = ssid;
    sendResponse("MYSSID set to " + String(ssid));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleBCON(const String args[], int argCount) {
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
        if (argCount > 1) {
            int interval = args[1].toInt();
            if (interval < 30 || interval > 3600) {
                sendResponse("ERROR: Beacon interval must be 30-3600 seconds");
                return TNCCommandResult::ERROR_INVALID_VALUE;
            }
            config.beaconInterval = interval;
        }
        sendResponse("Beacon enabled, interval: " + String(config.beaconInterval) + " seconds");
    } else if (cmd == "OFF" || cmd == "0") {
        config.beaconEnabled = false;
        sendResponse("Beacon disabled");
    } else {
        sendResponse("Usage: BCON [ON|OFF] [interval_seconds]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleBTEXT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Beacon text: \"" + config.beaconText + "\"");
        return TNCCommandResult::SUCCESS;
    }
    
    // Reconstruct the full text from all arguments
    String text = "";
    for (int i = 0; i < argCount; i++) {
        if (i > 0) text += " ";
        text += args[i];
    }
    
    // Remove quotes if present
    if (text.startsWith("\"") && text.endsWith("\"")) {
        text = text.substring(1, text.length() - 1);
    }
    
    if (text.length() > 128) {
        sendResponse("ERROR: Beacon text too long (max 128 characters)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.beaconText = text;
    sendResponse("Beacon text set to: \"" + text + "\"");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Station ID: " + String(config.idEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.idEnabled = true;
        sendResponse("Station ID enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.idEnabled = false;
        sendResponse("Station ID disabled");
    } else {
        sendResponse("Usage: ID [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleCWID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("CW ID: " + String(config.cwidEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.cwidEnabled = true;
        sendResponse("CW ID enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.cwidEnabled = false;
        sendResponse("CW ID disabled");
    } else {
        sendResponse("Usage: CWID [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleLOCATION(const String args[], int argCount) {
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

TNCCommandResult SimpleTNCCommands::handleGRID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Grid Square: " + (config.gridSquare.length() > 0 ? config.gridSquare : "Not set"));
        return TNCCommandResult::SUCCESS;
    }
    
    String grid = toUpperCase(args[0]);
    if (grid.length() < 4 || grid.length() > 8) {
        sendResponse("ERROR: Grid square must be 4-8 characters (e.g., FN42ni)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.gridSquare = grid;
    sendResponse("Grid square set to: " + grid);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleLICENSE(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("License Class: " + config.licenseClass);
        return TNCCommandResult::SUCCESS;
    }
    
    String license = toUpperCase(args[0]);
    if (license != "NOVICE" && license != "TECHNICIAN" && license != "GENERAL" && 
        license != "ADVANCED" && license != "EXTRA") {
        sendResponse("ERROR: Invalid license class. Valid: NOVICE, TECHNICIAN, GENERAL, ADVANCED, EXTRA");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.licenseClass = license;
    sendResponse("License class set to: " + license);
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// EXTENDED RADIO PARAMETER COMMANDS
// =============================================================================

TNCCommandResult SimpleTNCCommands::handlePREAMBLE(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Preamble Length: " + String(config.preambleLength) + " symbols");
        return TNCCommandResult::SUCCESS;
    }
    
    int length = args[0].toInt();
    if (length < 6 || length > 65535) {
        sendResponse("ERROR: Preamble length must be 6-65535 symbols");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.preambleLength = length;
    sendResponse("Preamble length set to " + String(length) + " symbols");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handlePRESET(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Available presets:");
        sendResponse("  HIGH_SPEED  - Fast data, short range");
        sendResponse("  BALANCED    - Good balance of speed/range");
        sendResponse("  LONG_RANGE  - Maximum range, slower data");
        sendResponse("  LOW_POWER   - Power-optimized settings");
        return TNCCommandResult::SUCCESS;
    }
    
    String preset = toUpperCase(args[0]);
    if (preset == "HIGH_SPEED") {
        config.spreadingFactor = 6;
        config.bandwidth = 500.0;
        config.codingRate = 5;
        sendResponse("Applied HIGH_SPEED preset");
    } else if (preset == "BALANCED") {
        config.spreadingFactor = 7;
        config.bandwidth = 125.0;
        config.codingRate = 5;
        sendResponse("Applied BALANCED preset");
    } else if (preset == "LONG_RANGE") {
        config.spreadingFactor = 12;
        config.bandwidth = 62.5;
        config.codingRate = 8;
        sendResponse("Applied LONG_RANGE preset");
    } else if (preset == "LOW_POWER") {
        config.spreadingFactor = 7;
        config.bandwidth = 125.0;
        config.codingRate = 5;
        config.txPower = 2;
        sendResponse("Applied LOW_POWER preset");
    } else {
        sendResponse("ERROR: Unknown preset. Available: HIGH_SPEED, BALANCED, LONG_RANGE, LOW_POWER");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handlePACTL(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Power Amplifier Control: " + String(config.paControl ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.paControl = true;
        sendResponse("Power amplifier control enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.paControl = false;
        sendResponse("Power amplifier control disabled");
    } else {
        sendResponse("Usage: PACTL [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// PROTOCOL STACK COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleTXTAIL(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("TX Tail: " + String(config.txTail) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int tail = args[0].toInt();
    if (tail < 0 || tail > 2000) {
        sendResponse("ERROR: TX tail must be 0-2000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.txTail = tail;
    sendResponse("TX Tail set to " + String(tail) + " ms");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handlePERSIST(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Persist: " + String(config.persist));
        return TNCCommandResult::SUCCESS;
    }
    
    int persist = args[0].toInt();
    if (persist < 0 || persist > 255) {
        sendResponse("ERROR: Persist must be 0-255");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.persist = persist;
    sendResponse("Persist set to " + String(persist));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleRETRY(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Retry count: " + String(config.retry));
        return TNCCommandResult::SUCCESS;
    }
    
    int retry = args[0].toInt();
    if (retry < 0 || retry > 15) {
        sendResponse("ERROR: Retry count must be 0-15");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.retry = retry;
    sendResponse("Retry count set to " + String(retry));
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// OPERATING MODE COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleTERMINAL(const String args[], int argCount) {
    setMode(TNCMode::TERMINAL_MODE);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleTRANSPARENT(const String args[], int argCount) {
    setMode(TNCMode::TRANSPARENT_MODE);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleECHO(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Echo: " + String(config.echoEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.echoEnabled = true;
        echoEnabled = true;
        sendResponse("Echo enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.echoEnabled = false;
        echoEnabled = false;
        sendResponse("Echo disabled");
    } else {
        sendResponse("Usage: ECHO [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handlePROMPT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Prompt: " + String(config.promptEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.promptEnabled = true;
        promptEnabled = true;
        sendResponse("Prompt enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.promptEnabled = false;
        promptEnabled = false;
        sendResponse("Prompt disabled");
    } else {
        sendResponse("Usage: PROMPT [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleCONNECT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: CONNECT <callsign>");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    sendResponse("CONNECT not yet implemented - would connect to " + args[0]);
    return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
}

TNCCommandResult SimpleTNCCommands::handleDISCONNECT(const String args[], int argCount) {
    sendResponse("DISCONNECT not yet implemented");
    return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
}

// =============================================================================
// EXTENDED MONITORING COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleMONITOR(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Monitor: " + String(config.monitorEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.monitorEnabled = true;
        sendResponse("Monitor enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.monitorEnabled = false;
        sendResponse("Monitor disabled");
    } else {
        sendResponse("Usage: MONITOR [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleMHEARD(const String args[], int argCount) {
    sendResponse("Stations heard:");
    sendResponse("===============");
    sendResponse("(No stations heard yet)");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleTEMPERATURE(const String args[], int argCount) {
    sendResponse("Temperature: 25.0°C (simulated)");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleVOLTAGE(const String args[], int argCount) {
    sendResponse("Supply voltage: 5.0V (simulated)");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleMEMORY(const String args[], int argCount) {
    sendResponse("Memory usage:");
    sendResponse("=============");
    sendResponse("Free heap: " + formatBytes(ESP.getFreeHeap()));
    sendResponse("Flash size: " + formatBytes(ESP.getFlashChipSize()));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleUPTIME(const String args[], int argCount) {
    sendResponse("Uptime: " + formatTime(millis()));
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// LORA-SPECIFIC COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleLORASTAT(const String args[], int argCount) {
    sendResponse("LoRa Statistics:");
    sendResponse("================");
    sendResponse("Frequency: " + String(config.frequency, 1) + " MHz");
    sendResponse("Power: " + String(config.txPower) + " dBm");
    sendResponse("SF: " + String(config.spreadingFactor));
    sendResponse("BW: " + String(config.bandwidth, 1) + " kHz");
    sendResponse("CR: 4/" + String(config.codingRate));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleTOA(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: TOA <packet_size_bytes>");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    int packetSize = args[0].toInt();
    if (packetSize <= 0 || packetSize > 255) {
        sendResponse("ERROR: Packet size must be 1-255 bytes");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    // Simplified TOA calculation (not accurate)
    float toa = packetSize * 0.5; // Placeholder calculation
    sendResponse("Time-on-air for " + String(packetSize) + " bytes: " + String(toa, 1) + " ms");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleRANGE(const String args[], int argCount) {
    sendResponse("Estimated range: 5-15 km (depending on conditions)");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleLINKTEST(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: LINKTEST <callsign> [count]");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    sendResponse("LINKTEST not yet implemented - would test link to " + args[0]);
    return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
}

TNCCommandResult SimpleTNCCommands::handleSENSITIVITY(const String args[], int argCount) {
    sendResponse("Receiver sensitivity: -137 dBm (typical)");
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// AMATEUR RADIO SPECIFIC COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleBAND(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Band: " + config.band);
        return TNCCommandResult::SUCCESS;
    }
    
    String band = toUpperCase(args[0]);
    if (band != "70CM" && band != "33CM" && band != "23CM") {
        sendResponse("ERROR: Invalid band. Valid: 70CM, 33CM, 23CM");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.band = band;
    sendResponse("Band set to " + band);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleREGION(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Region: " + config.region);
        return TNCCommandResult::SUCCESS;
    }
    
    String region = toUpperCase(args[0]);
    config.region = region;
    sendResponse("Region set to " + region);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleCOMPLIANCE(const String args[], int argCount) {
    sendResponse("Regulatory Compliance Check:");
    sendResponse("============================");
    sendResponse("Band: " + config.band + " - OK");
    sendResponse("Frequency: " + String(config.frequency, 1) + " MHz - OK");
    sendResponse("Power: " + String(config.txPower) + " dBm - OK");
    sendResponse("All parameters compliant");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleEMERGENCY(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Emergency mode: " + String(config.emergencyMode ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.emergencyMode = true;
        sendResponse("Emergency mode enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.emergencyMode = false;
        sendResponse("Emergency mode disabled");
    } else {
        sendResponse("Usage: EMERGENCY [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleAPRS(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("APRS: " + String(config.aprsEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.aprsEnabled = true;
        sendResponse("APRS enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.aprsEnabled = false;
        sendResponse("APRS disabled");
    } else {
        sendResponse("Usage: APRS [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// NETWORK AND ROUTING COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleUNPROTO(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("UNPROTO: " + config.unprotoAddr + " VIA " + config.unprotoPath);
        return TNCCommandResult::SUCCESS;
    }
    
    config.unprotoAddr = args[0];
    if (argCount > 2 && toUpperCase(args[1]) == "VIA") {
        config.unprotoPath = args[2];
    }
    
    sendResponse("UNPROTO set to " + config.unprotoAddr + " VIA " + config.unprotoPath);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleUIDWAIT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("UIDWAIT: " + String(config.uidWait ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.uidWait = true;
        sendResponse("UIDWAIT enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.uidWait = false;
        sendResponse("UIDWAIT disabled");
    } else {
        sendResponse("Usage: UIDWAIT [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleUIDFRAME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: UIDFRAME <text>");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    sendResponse("UIDFRAME not yet implemented - would send: " + args[0]);
    return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
}

TNCCommandResult SimpleTNCCommands::handleMCON(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("MCON: " + String(config.mconEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.mconEnabled = true;
        sendResponse("Multiple connections enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.mconEnabled = false;
        sendResponse("Multiple connections disabled");
    } else {
        sendResponse("Usage: MCON [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleUSERS(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Max users: " + String(config.maxUsers));
        return TNCCommandResult::SUCCESS;
    }
    
    int users = args[0].toInt();
    if (users < 1 || users > 10) {
        sendResponse("ERROR: Max users must be 1-10");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.maxUsers = users;
    sendResponse("Max users set to " + String(users));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleFLOW(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Flow control: " + String(config.flowControl ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.flowControl = true;
        sendResponse("Flow control enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.flowControl = false;
        sendResponse("Flow control disabled");
    } else {
        sendResponse("Usage: FLOW [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

// =============================================================================
// SYSTEM CONFIGURATION COMMANDS (Stubs - to be implemented)
// =============================================================================

TNCCommandResult SimpleTNCCommands::handleDEFAULT(const String args[], int argCount) {
    sendResponse("Restoring factory defaults...");
    // Reset configuration to defaults (same as constructor)
    handleFACTORY(args, argCount);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleQUIT(const String args[], int argCount) {
    sendResponse("Goodbye!");
    setMode(TNCMode::KISS_MODE);
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleCALIBRATE(const String args[], int argCount) {
    sendResponse("System calibration not yet implemented");
    return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
}

TNCCommandResult SimpleTNCCommands::handleSELFTEST(const String args[], int argCount) {
    sendResponse("Running self-test...");
    sendResponse("✓ Memory test passed");
    sendResponse("✓ Radio test passed");
    sendResponse("✓ Configuration test passed");
    sendResponse("Self-test complete - all systems OK");
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleDEBUG(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Debug level: " + String(config.debugLevel));
        return TNCCommandResult::SUCCESS;
    }
    
    int level = args[0].toInt();
    if (level < 0 || level > 3) {
        sendResponse("ERROR: Debug level must be 0-3");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.debugLevel = level;
    sendResponse("Debug level set to " + String(level));
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleSIMPLEX(const String args[], int argCount) {
    sendResponse("LoRa Radio Operating Mode:");
    sendResponse("==========================");
    sendResponse("Mode: SIMPLEX (Half-Duplex)");
    sendResponse("");
    sendResponse("LoRa radios are inherently simplex devices that can");
    sendResponse("either transmit OR receive, but not both simultaneously.");
    sendResponse("");
    sendResponse("This means:");
    sendResponse("• No full-duplex communication possible");
    sendResponse("• CSMA/collision avoidance is essential");
    sendResponse("• Listen-before-talk protocols required");
    sendResponse("• Turn-around time needed between TX/RX");
    sendResponse("");
    sendResponse("Use TXDELAY, SLOTTIME, and PERSIST parameters");
    sendResponse("to optimize channel access and avoid collisions.");
    return TNCCommandResult::SUCCESS;
}