/**
 * @file TNCCommands.cpp
 * @brief TNC command system implementation
 * @author LoRaTNCX Project
 * @date October 29, 2025
 */

#include "TNCCommands.h"
#include "LoRaRadio.h"
#include "esp_system.h"
#include <Preferences.h>

// Command handlers are implemented in separate translation units under
// lib/tnc_commands/src/commands to simplify maintenance.
TNCCommands::TNCCommands() 
    : currentMode(TNCMode::COMMAND_MODE), echoEnabled(true), promptEnabled(true), radio(nullptr) {
    
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
    config.lineEndingCR = true;
    config.lineEndingLF = true;
    
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
    
    // Initialize routing table
    routeCount = 0;
    for (int i = 0; i < MAX_ROUTES; i++) {
        routingTable[i].destination = "";
        routingTable[i].nextHop = "";
        routingTable[i].hops = 0;
        routingTable[i].quality = 0.0;
        routingTable[i].lastUsed = 0;
        routingTable[i].lastUpdated = 0;
        routingTable[i].isActive = false;
    }
    
    // Initialize node table
    nodeCount = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        nodeTable[i].callsign = "";
        nodeTable[i].ssid = 0;
        nodeTable[i].lastRSSI = 0.0;
        nodeTable[i].lastSNR = 0.0;
        nodeTable[i].lastHeard = 0;
        nodeTable[i].firstHeard = 0;
        nodeTable[i].packetCount = 0;
        nodeTable[i].lastPacket = "";
        nodeTable[i].isBeacon = false;
    }
    
    // Initialize connection table
    activeConnections = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].remoteCall = "";
        connections[i].remoteSSID = 0;
        connections[i].state = DISCONNECTED;
        connections[i].connectTime = 0;
        connections[i].lastActivity = 0;
        connections[i].vs = 0;
        connections[i].vr = 0;
        connections[i].va = 0;
        connections[i].retryCount = 0;
        connections[i].pollBit = false;
    }
}

TNCCommandResult TNCCommands::processCommand(const String& commandLine) {
    if (commandLine.length() == 0) {
        return TNCCommandResult::SUCCESS;
    }
    
    // Check if we're in converse mode (connected to another station)
    if (isInConverseMode()) {
        // Handle special commands that work in converse mode
        String upperCommand = toUpperCase(commandLine);
        
        if (commandLine.startsWith("/DISC") || commandLine.startsWith("/D") || 
            upperCommand == "DISCONNECT" || upperCommand == "DISC") {
            // Disconnect command (both /DISC and regular DISCONNECT work)
            bool disconnected = false;
            for (int i = 0; i < activeConnections; i++) {
                if (connections[i].state == CONNECTED) {
                    if (sendDisconnectFrame(i)) {
                        connections[i].state = DISCONNECTING;
                        sendResponse("*** DISCONNECTING from " + connections[i].remoteCall + 
                                   (connections[i].remoteSSID > 0 ? "-" + String(connections[i].remoteSSID) : "") + " ***");
                        disconnected = true;
                    }
                }
            }
            if (disconnected) {
                sendResponse("Returning to command mode");
                sendPrompt();
            }
            return TNCCommandResult::SUCCESS;
        } else if (upperCommand == "CMD" || upperCommand == "COMMAND" || upperCommand == "QUIT") {
            // Force return to command mode without sending DISC
            sendResponse("*** FORCING RETURN TO COMMAND MODE ***");
            sendResponse("Warning: Connections may still be active on remote end");
            
            // Mark all connections as disconnected locally
            for (int i = 0; i < activeConnections; i++) {
                if (connections[i].state == CONNECTED) {
                    connections[i].state = DISCONNECTED;
                    sendResponse("Local connection to " + connections[i].remoteCall + 
                               (connections[i].remoteSSID > 0 ? "-" + String(connections[i].remoteSSID) : "") + " closed");
                }
            }
            
            sendResponse("Returning to command mode");
            sendPrompt();
            return TNCCommandResult::SUCCESS;
        } else if (upperCommand == "CONNECT" || upperCommand.startsWith("CONNECT ")) {
            // Allow new connections from converse mode
            sendResponse("Processing CONNECT command from converse mode...");
            // Fall through to regular command processing
        } else if (commandLine.startsWith("/HELP") || commandLine.startsWith("/?") || upperCommand == "HELP") {
            // Help in converse mode
            sendResponse("=== CONVERSE MODE HELP ===");
            sendResponse("You are connected and in chat mode.");
            sendResponse("");
            sendResponse("Disconnect commands:");
            sendResponse("  /DISC or /D     - Send disconnect and return to command mode");
            sendResponse("  DISCONNECT      - Same as /DISC");
            sendResponse("  CMD or QUIT     - Force return to command mode (no disconnect sent)");
            sendResponse("");
            sendResponse("Other commands:");
            sendResponse("  CONNECT <call>  - Connect to additional station");
            sendResponse("  /HELP or HELP  - Show this help");
            sendResponse("");
            sendResponse("Anything else is sent as a chat message to connected station(s)");
            return TNCCommandResult::SUCCESS;
        } else if (upperCommand == "STATUS" || upperCommand == "NODES" || upperCommand == "STATS") {
            // Allow some status commands in converse mode
            sendResponse("Processing " + upperCommand + " command from converse mode...");
            // Fall through to regular command processing  
        } else {
            // Regular chat message
            if (sendChatMessage(commandLine)) {
                // Echo our own message
                sendResponse("[" + config.myCall + 
                           (config.mySSID > 0 ? "-" + String(config.mySSID) : "") + "] " + commandLine);
            } else {
                sendResponse("*** ERROR: Failed to send message ***");
            }
            return TNCCommandResult::SUCCESS;
        }
    }
    
    // Regular command mode processing
    
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
        result = TNCCommandResult::SUCCESS_SILENT;
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
    } else if (command == "SAVED") {
        result = handleSAVED(cmdArgs, cmdArgCount);
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
    } else if (command == "LINECR") {
        result = handleLINECR(cmdArgs, cmdArgCount);
    } else if (command == "LINELF") {
        result = handleLINELF(cmdArgs, cmdArgCount);
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
        case TNCCommandResult::SUCCESS_SILENT:
            // No response - silent success (used for KISS mode entry)
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

void TNCCommands::setMode(TNCMode mode) {
    TNCMode previousMode = currentMode;
    currentMode = mode;
    
    // Only send mode change message when NOT entering KISS mode
    // AND when NOT exiting KISS mode (KISS mode must be completely silent)
    if (mode != TNCMode::KISS_MODE && previousMode != TNCMode::KISS_MODE) {
        String modeStr = getModeString();
        sendResponse("Entering " + modeStr + " mode");
    }
    
    // Configure interface based on mode
    switch (mode) {
        case TNCMode::KISS_MODE:
            echoEnabled = false;
            promptEnabled = false;
            break;
        case TNCMode::COMMAND_MODE:
            echoEnabled = config.echoEnabled;
            promptEnabled = config.promptEnabled;
            break;
        default:
            break;
    }

    // Only send prompt when NOT coming from KISS mode (to maintain silence)
    if (promptEnabled && previousMode != TNCMode::KISS_MODE) {
        sendPrompt();
    }
}

String TNCCommands::getModeString() const {
    switch (currentMode) {
        case TNCMode::KISS_MODE: return "KISS";
        case TNCMode::COMMAND_MODE: return "COMMAND";
        case TNCMode::TERMINAL_MODE: return "TERMINAL";
        case TNCMode::TRANSPARENT_MODE: return "TRANSPARENT";
        default: return "UNKNOWN";
    }
}

void TNCCommands::sendResponse(const String& response) {
    Serial.print(response);
    if (config.lineEndingCR) {
        Serial.write('\r');
    }
    if (config.lineEndingLF) {
        Serial.write('\n');
    }
}

void TNCCommands::sendPrompt() {
    switch (currentMode) {
        case TNCMode::COMMAND_MODE:
            Serial.print(TNC_COMMAND_PROMPT);
            break;
        default:
            break;
    }
}

void TNCCommands::setRadio(LoRaRadio* radioPtr) {
    radio = radioPtr;
}

bool TNCCommands::loadConfigurationFromFlash() {
    Preferences preferences;
    if (!preferences.begin("tnc_config", true)) {
        return false;
    }
    
    // Only load if configuration exists
    if (!preferences.isKey("myCall")) {
        preferences.end();
        return false;  // No saved config
    }
    
    // Station configuration
    config.myCall = preferences.getString("myCall", "NOCALL");
    config.mySSID = preferences.getUChar("mySSID", 0);
    config.beaconText = preferences.getString("beaconText", "LoRaTNCX Test Station");
    config.idEnabled = preferences.getBool("idEnabled", true);
    config.cwidEnabled = preferences.getBool("cwidEnabled", false);
    config.latitude = preferences.getFloat("latitude", 0.0);
    config.longitude = preferences.getFloat("longitude", 0.0);
    config.altitude = preferences.getInt("altitude", 0);
    config.gridSquare = preferences.getString("gridSquare", "");
    config.licenseClass = preferences.getString("licenseClass", "GENERAL");
    
    // Radio parameters
    config.frequency = preferences.getFloat("frequency", 915.0);
    config.txPower = preferences.getInt("txPower", 10);
    config.spreadingFactor = preferences.getUChar("spreadingFactor", 7);
    config.bandwidth = preferences.getFloat("bandwidth", 125.0);
    config.codingRate = preferences.getUChar("codingRate", 5);
    config.syncWord = preferences.getUChar("syncWord", 0x12);
    config.preambleLength = preferences.getUChar("preambleLength", 8);
    config.paControl = preferences.getBool("paControl", true);
    
    // Protocol stack
    config.txDelay = preferences.getUShort("txDelay", 300);
    config.txTail = preferences.getUShort("txTail", 100);
    config.persist = preferences.getUChar("persist", 63);
    config.slotTime = preferences.getUShort("slotTime", 100);
    config.respTime = preferences.getUShort("respTime", 1000);
    config.maxFrame = preferences.getUChar("maxFrame", 4);
    config.frack = preferences.getUShort("frack", 3000);
    config.retry = preferences.getUChar("retry", 10);
    
    // Operating modes
    config.echoEnabled = preferences.getBool("echoEnabled", true);
    config.promptEnabled = preferences.getBool("promptEnabled", true);
    config.monitorEnabled = preferences.getBool("monitorEnabled", false);
    config.lineEndingCR = preferences.getBool("lineEndingCR", true);
    config.lineEndingLF = preferences.getBool("lineEndingLF", true);
    
    // Beacon and digi
    config.beaconEnabled = preferences.getBool("beaconEnabled", false);
    config.beaconInterval = preferences.getUShort("beaconInterval", 600);
    config.digiEnabled = preferences.getBool("digiEnabled", false);
    config.digiPath = preferences.getUChar("digiPath", 4);

    // System
    config.debugLevel = preferences.getUChar("debugLevel", config.debugLevel);

    preferences.end();

    echoEnabled = config.echoEnabled;
    promptEnabled = config.promptEnabled;

    return true;
}

bool TNCCommands::saveConfigurationToFlash() {
    Preferences preferences;
    if (!preferences.begin("tnc_config", false)) {
        return false;
    }
    
    // Station configuration
    preferences.putString("myCall", config.myCall);
    preferences.putUChar("mySSID", config.mySSID);
    preferences.putString("beaconText", config.beaconText);
    preferences.putBool("idEnabled", config.idEnabled);
    preferences.putBool("cwidEnabled", config.cwidEnabled);
    preferences.putFloat("latitude", config.latitude);
    preferences.putFloat("longitude", config.longitude);
    preferences.putInt("altitude", config.altitude);
    preferences.putString("gridSquare", config.gridSquare);
    preferences.putString("licenseClass", config.licenseClass);
    
    // Radio parameters
    preferences.putFloat("frequency", config.frequency);
    preferences.putInt("txPower", config.txPower);
    preferences.putUChar("spreadingFactor", config.spreadingFactor);
    preferences.putFloat("bandwidth", config.bandwidth);
    preferences.putUChar("codingRate", config.codingRate);
    preferences.putUChar("syncWord", config.syncWord);
    preferences.putUChar("preambleLength", config.preambleLength);
    preferences.putBool("paControl", config.paControl);
    
    // Protocol stack
    preferences.putUShort("txDelay", config.txDelay);
    preferences.putUShort("txTail", config.txTail);
    preferences.putUChar("persist", config.persist);
    preferences.putUShort("slotTime", config.slotTime);
    preferences.putUShort("respTime", config.respTime);
    preferences.putUChar("maxFrame", config.maxFrame);
    preferences.putUShort("frack", config.frack);
    preferences.putUChar("retry", config.retry);
    
    // Operating modes
    preferences.putBool("echoEnabled", config.echoEnabled);
    preferences.putBool("promptEnabled", config.promptEnabled);
    preferences.putBool("monitorEnabled", config.monitorEnabled);
    preferences.putBool("lineEndingCR", config.lineEndingCR);
    preferences.putBool("lineEndingLF", config.lineEndingLF);
    
    // Beacon and digi
    preferences.putBool("beaconEnabled", config.beaconEnabled);
    preferences.putUShort("beaconInterval", config.beaconInterval);
    preferences.putBool("digiEnabled", config.digiEnabled);
    preferences.putUChar("digiPath", config.digiPath);

    // System
    preferences.putUChar("debugLevel", config.debugLevel);
    
    preferences.end();
    return true;
}

int TNCCommands::parseCommandLine(const String& line, String args[], int maxArgs) {
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

String TNCCommands::toUpperCase(const String& str) {
    String result = str;
    result.toUpperCase();
    return result;
}











String TNCCommands::formatTime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    String result = "";
    if (hours > 0) result += String(hours) + "h ";
    if (minutes % 60 > 0) result += String(minutes % 60) + "m ";
    result += String(seconds % 60) + "s";
    
    return result;
}

String TNCCommands::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024) + " KB";
    else return String(bytes / (1024 * 1024)) + " MB";
}

// =============================================================================
// RADIO CONFIGURATION COMMANDS
// =============================================================================













// =============================================================================
// NETWORK AND ROUTING COMMANDS
// =============================================================================









// =============================================================================
// PROTOCOL TIMING COMMANDS
// =============================================================================











// =============================================================================
// STATISTICS AND MONITORING COMMANDS
// =============================================================================









// =============================================================================
// CONFIGURATION MANAGEMENT COMMANDS
// =============================================================================









// =============================================================================
// TESTING AND DIAGNOSTIC COMMANDS
// =============================================================================









// =============================================================================
// STATION CONFIGURATION COMMANDS
// =============================================================================

















// =============================================================================
// EXTENDED RADIO PARAMETER COMMANDS
// =============================================================================







// =============================================================================
// PROTOCOL STACK COMMANDS (Stubs - to be implemented)
// =============================================================================







// =============================================================================
// OPERATING MODE COMMANDS (Stubs - to be implemented)
// =============================================================================













// =============================================================================
// EXTENDED MONITORING COMMANDS (Stubs - to be implemented)
// =============================================================================













// =============================================================================
// LORA-SPECIFIC COMMANDS (Stubs - to be implemented)
// =============================================================================











// =============================================================================
// AMATEUR RADIO SPECIFIC COMMANDS (Stubs - to be implemented)
// =============================================================================











// =============================================================================
// NETWORK AND ROUTING COMMANDS (Stubs - to be implemented)
// =============================================================================













// =============================================================================
// SYSTEM CONFIGURATION COMMANDS (Stubs - to be implemented)
// =============================================================================













// =============================================================================
// UTILITY METHODS
// =============================================================================

TNCCommandResult TNCCommands::transmitBeacon() {
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    // Create beacon packet in APRS-style format
    String beacon = "BEACON:" + config.myCall;
    if (config.mySSID > 0) {
        beacon += "-" + String(config.mySSID);
    }
    beacon += ">APRS";
    
    // Add path if configured  
    if (config.unprotoPath.length() > 0) {
        beacon += "," + config.unprotoPath;
    }
    
    beacon += ":>";
    
    // Add position if configured (APRS position format)
    if (config.latitude != 0.0 || config.longitude != 0.0) {
        // Simple position format (not full APRS format, but readable)
        beacon += "Lat:" + String(config.latitude, 6) + " Lon:" + String(config.longitude, 6);
        if (config.altitude > 0) {
            beacon += " Alt:" + String(config.altitude) + "m";
        }
        beacon += " ";
    }
    
    // Add beacon text
    beacon += config.beaconText;
    
    // Add timestamp
    beacon += " [" + String(millis()/1000) + "s]";
    
    // Validate beacon size
    if (beacon.length() > 240) {
        sendResponse("ERROR: Beacon too large (" + String(beacon.length()) + " bytes, max 240)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    // Transmit beacon
    if (radio->transmit(beacon)) {
        sendResponse("Beacon transmitted (" + String(beacon.length()) + " bytes)");
        sendResponse("Beacon: " + beacon);
        
        // Update statistics
        stats.packetsTransmitted++;
        stats.bytesTransmitted += beacon.length();
        
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Beacon transmission failed");
        stats.packetErrors++;
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
}

void TNCCommands::updateNodeTable(const String& callsign, uint8_t ssid, float rssi, float snr, const String& packet, bool isBeacon) {
    // Look for existing entry
    int existingIndex = -1;
    for (int i = 0; i < nodeCount; i++) {
        if (nodeTable[i].callsign == callsign && nodeTable[i].ssid == ssid) {
            existingIndex = i;
            break;
        }
    }
    
    unsigned long now = millis();
    
    if (existingIndex >= 0) {
        // Update existing entry
        nodeTable[existingIndex].lastRSSI = rssi;
        nodeTable[existingIndex].lastSNR = snr;
        nodeTable[existingIndex].lastHeard = now;
        nodeTable[existingIndex].packetCount++;
        nodeTable[existingIndex].lastPacket = packet.length() > 50 ? packet.substring(0, 50) : packet;
        nodeTable[existingIndex].isBeacon = isBeacon;
    } else {
        // Add new entry if there's space
        if (nodeCount < MAX_NODES) {
            nodeTable[nodeCount].callsign = callsign;
            nodeTable[nodeCount].ssid = ssid;
            nodeTable[nodeCount].lastRSSI = rssi;
            nodeTable[nodeCount].lastSNR = snr;
            nodeTable[nodeCount].lastHeard = now;
            nodeTable[nodeCount].firstHeard = now;
            nodeTable[nodeCount].packetCount = 1;
            nodeTable[nodeCount].lastPacket = packet.length() > 50 ? packet.substring(0, 50) : packet;
            nodeTable[nodeCount].isBeacon = isBeacon;
            nodeCount++;
        }
        // If table is full, we could replace the oldest entry, but for now just ignore
    }
}

bool TNCCommands::shouldDigipeat(const String& path) {
    if (!config.digiEnabled) {
        return false;
    }
    
    // Parse path to see if we should digipeat
    // Path format: "DEST>SOURCE,VIA1,VIA2*,VIA3"
    // * indicates the path has been used by that digipeater
    
    int commaIndex = path.indexOf(',');
    if (commaIndex == -1) {
        return false; // No digipeater path
    }
    
    String viaPath = path.substring(commaIndex + 1);
    
    // Split via path by commas
    int startPos = 0;
    int commaPos = 0;
    
    while ((commaPos = viaPath.indexOf(',', startPos)) != -1 || startPos < viaPath.length()) {
        String hop;
        if (commaPos != -1) {
            hop = viaPath.substring(startPos, commaPos);
        } else {
            hop = viaPath.substring(startPos);  // Last hop
        }
        
        hop.trim();
        
        // Check if this hop has already been used (marked with *)
        if (hop.endsWith("*")) {
            startPos = commaPos + 1;
            continue;  // Skip used hops
        }
        
        // Check if this is our callsign
        String myCallWithSSID = config.myCall;
        if (config.mySSID > 0) {
            myCallWithSSID += "-" + String(config.mySSID);
        }
        
        if (hop == myCallWithSSID) {
            return true;  // Direct addressing to us
        }
        
        // Check for standard aliases
        if (hop == "WIDE1-1" || hop == "WIDE2-1" || 
            hop.startsWith("WIDE") && hop.indexOf('-') > 0) {
            return true;  // Standard WIDEn-N alias
        }
        
        // For WIDEn-N, we only digipeat the first unused hop
        break;
    }
    
    return false;
}

String TNCCommands::processDigipeatPath(const String& path) {
    // Process the path for digipeating
    // Input: "DEST>SOURCE,VIA1,VIA2,VIA3"
    // Output: "DEST>SOURCE,VIA1*,VIA2,VIA3" (marking the hop we used)
    
    String result = path;
    
    int commaIndex = result.indexOf(',');
    if (commaIndex == -1) {
        return result; // No via path to process
    }
    
    String beforeVia = result.substring(0, commaIndex + 1);
    String viaPath = result.substring(commaIndex + 1);
    
    // Find the first unused hop that we should digipeat
    int startPos = 0;
    int commaPos = 0;
    String newViaPath = "";
    bool found = false;
    
    while ((commaPos = viaPath.indexOf(',', startPos)) != -1 || startPos < viaPath.length()) {
        String hop;
        if (commaPos != -1) {
            hop = viaPath.substring(startPos, commaPos);
        } else {
            hop = viaPath.substring(startPos);  // Last hop
        }
        
        hop.trim();
        
        if (newViaPath.length() > 0) {
            newViaPath += ",";
        }
        
        // If this hop is already used (has *), keep it as is
        if (hop.endsWith("*")) {
            newViaPath += hop;
        } 
        // If this is the first unused hop we should digipeat
        else if (!found && shouldProcessHop(hop)) {
            found = true;
            
            // Add our callsign and mark the original hop as used
            String myCallWithSSID = config.myCall;
            if (config.mySSID > 0) {
                myCallWithSSID += "-" + String(config.mySSID);
            }
            newViaPath += myCallWithSSID + "*";
            
            // Process WIDEn-N decrementation
            if (hop.startsWith("WIDE") && hop.indexOf('-') > 0) {
                int dashPos = hop.indexOf('-');
                if (dashPos > 0) {
                    String prefix = hop.substring(0, dashPos + 1);
                    int hopsLeft = hop.substring(dashPos + 1).toInt() - 1;
                    if (hopsLeft > 0) {
                        newViaPath += "," + prefix + String(hopsLeft);
                    }
                }
            }
        }
        // Otherwise, keep the hop as-is
        else {
            newViaPath += hop;
        }
        
        if (commaPos == -1) break;
        startPos = commaPos + 1;
    }
    
    return beforeVia + newViaPath;
}

bool TNCCommands::shouldProcessHop(const String& hop) {
    // Check if we should process this specific hop
    String myCallWithSSID = config.myCall;
    if (config.mySSID > 0) {
        myCallWithSSID += "-" + String(config.mySSID);
    }
    
    // Direct callsign match
    if (hop == myCallWithSSID) {
        return true;
    }
    
    // WIDE aliases
    if (hop == "WIDE1-1" || hop == "WIDE2-1" || hop == "WIDE2-2") {
        return true;
    }
    
    // General WIDEn-N pattern
    if (hop.startsWith("WIDE") && hop.indexOf('-') > 0) {
        return true;
    }
    
    return false;
}

bool TNCCommands::sendDisconnectFrame(int connectionIndex) {
    if (!radio || connectionIndex < 0 || connectionIndex >= activeConnections) {
        return false;
    }
    
    ConnectionInfo& conn = connections[connectionIndex];
    
    // Send DISC (Disconnect) frame
    String disconnectFrame = "DISC:" + config.myCall;
    if (config.mySSID > 0) {
        disconnectFrame += "-" + String(config.mySSID);
    }
    disconnectFrame += ">" + conn.remoteCall;
    if (conn.remoteSSID > 0) {
        disconnectFrame += "-" + String(conn.remoteSSID);
    }
    disconnectFrame += ":DISCONNECT_REQUEST:" + String(millis());
    
    if (radio->transmit(disconnectFrame)) {
        // Update statistics
        stats.packetsTransmitted++;
        stats.bytesTransmitted += disconnectFrame.length();
        return true;
    } else {
        stats.packetErrors++;
        return false;
    }
}

void TNCCommands::processIncomingFrame(const String& frame, float rssi, float snr) {
    // Parse incoming AX.25-style frames and handle connection management
    
    if (frame.startsWith("SABM:")) {
        // Incoming connection request
        // Format: SABM:sourceCall>destCall:CONNECT_REQUEST:timestamp
        int colonPos = frame.indexOf(':', 5); // Skip "SABM:"
        if (colonPos > 0) {
            String addressing = frame.substring(5, colonPos);
            int gtPos = addressing.indexOf('>');
            if (gtPos > 0) {
                String sourceCallWithSSID = addressing.substring(0, gtPos);
                String destCallWithSSID = addressing.substring(gtPos + 1);
                
                String sourceCall, destCall;
                uint8_t sourceSSID, destSSID;
                parseCallsign(sourceCallWithSSID, sourceCall, sourceSSID);
                parseCallsign(destCallWithSSID, destCall, destSSID);
                
                // Check if this is for us
                if (destCall == config.myCall && destSSID == config.mySSID) {
                    sendResponse("*** INCOMING CONNECTION from " + sourceCallWithSSID + " ***");
                    
                    // Find or create connection slot
                    int connIndex = findConnection(sourceCall, sourceSSID);
                    if (connIndex == -1 && activeConnections < MAX_CONNECTIONS) {
                        connIndex = activeConnections++;
                        connections[connIndex].remoteCall = sourceCall;
                        connections[connIndex].remoteSSID = sourceSSID;
                        connections[connIndex].state = CONNECTED;
                        connections[connIndex].connectTime = millis();
                        connections[connIndex].lastActivity = millis();
                        connections[connIndex].vs = 0;
                        connections[connIndex].vr = 0;
                        connections[connIndex].va = 0;
                        connections[connIndex].retryCount = 0;
                        
                        // Send UA (Unnumbered Acknowledgment) response
                        if (sendUAFrame(sourceCall, sourceSSID)) {
                            sendResponse("Connection established with " + sourceCallWithSSID);
                            sendResponse("Entering converse mode. Type /DISC to disconnect.");
                            sendResponse("*** CONNECTED ***");
                        } else {
                            sendResponse("Failed to send UA response");
                        }
                    } else if (connIndex >= 0) {
                        // Already connected, send UA anyway
                        sendUAFrame(sourceCall, sourceSSID);
                    } else {
                        sendResponse("Connection table full, rejecting connection");
                        // Could send DM (Disconnect Mode) here
                    }
                }
            }
        }
    } else if (frame.startsWith("UA:")) {
        // Unnumbered Acknowledgment - connection accepted
        // Format: UA:sourceCall>destCall:CONNECT_ACCEPTED:timestamp
        int colonPos = frame.indexOf(':', 3); // Skip "UA:"
        if (colonPos > 0) {
            String addressing = frame.substring(3, colonPos);
            int gtPos = addressing.indexOf('>');
            if (gtPos > 0) {
                String sourceCallWithSSID = addressing.substring(0, gtPos);
                String destCallWithSSID = addressing.substring(gtPos + 1);
                
                String sourceCall;
                uint8_t sourceSSID;
                parseCallsign(sourceCallWithSSID, sourceCall, sourceSSID);
                
                // Find the connection we initiated
                int connIndex = findConnection(sourceCall, sourceSSID);
                if (connIndex >= 0 && connections[connIndex].state == CONNECTING) {
                    connections[connIndex].state = CONNECTED;
                    connections[connIndex].lastActivity = millis();
                    
                    sendResponse("*** CONNECTION ESTABLISHED with " + sourceCallWithSSID + " ***");
                    sendResponse("Entering converse mode. Type /DISC to disconnect.");
                    sendResponse("*** CONNECTED ***");
                }
            }
        }
    } else if (frame.startsWith("DISC:")) {
        // Disconnect request
        // Format: DISC:sourceCall>destCall:DISCONNECT_REQUEST:timestamp
        int colonPos = frame.indexOf(':', 5); // Skip "DISC:"
        if (colonPos > 0) {
            String addressing = frame.substring(5, colonPos);
            int gtPos = addressing.indexOf('>');
            if (gtPos > 0) {
                String sourceCallWithSSID = addressing.substring(0, gtPos);
                String sourceCall;
                uint8_t sourceSSID;
                parseCallsign(sourceCallWithSSID, sourceCall, sourceSSID);
                
                // Find and close the connection
                int connIndex = findConnection(sourceCall, sourceSSID);
                if (connIndex >= 0) {
                    sendResponse("*** DISCONNECTED by " + sourceCallWithSSID + " ***");
                    connections[connIndex].state = DISCONNECTED;
                    
                    // Send UA to acknowledge the disconnect
                    sendUAFrame(sourceCall, sourceSSID);
                    
                    sendResponse("Returning to command mode");
                    sendPrompt();
                }
            }
        }
    } else if (frame.startsWith("I:")) {
        // Information frame (chat message)
        // Format: I:sourceCall>destCall:message
        int colonPos = frame.indexOf(':', 2); // Skip "I:"
        if (colonPos > 0) {
            String addressing = frame.substring(2, colonPos);
            String message = frame.substring(colonPos + 1);
            
            int gtPos = addressing.indexOf('>');
            if (gtPos > 0) {
                String sourceCallWithSSID = addressing.substring(0, gtPos);
                sendResponse("[" + sourceCallWithSSID + "] " + message);
            }
        }
    }
    
    // Update node table for all received frames
    if (frame.indexOf('>') > 0) {
        String sourceCall;
        uint8_t sourceSSID;
        int gtPos = frame.indexOf('>');
        if (gtPos > 0) {
            String addressing = frame.substring(frame.indexOf(':') + 1, gtPos);
            parseCallsign(addressing, sourceCall, sourceSSID);
            updateNodeTable(sourceCall, sourceSSID, rssi, snr, frame, false);
        }
    }
}

void TNCCommands::parseCallsign(const String& callsignWithSSID, String& callsign, uint8_t& ssid) {
    int dashPos = callsignWithSSID.indexOf('-');
    if (dashPos > 0) {
        callsign = callsignWithSSID.substring(0, dashPos);
        ssid = callsignWithSSID.substring(dashPos + 1).toInt();
    } else {
        callsign = callsignWithSSID;
        ssid = 0;
    }
}

int TNCCommands::findConnection(const String& remoteCall, uint8_t remoteSSID) {
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].remoteCall == remoteCall && 
            connections[i].remoteSSID == remoteSSID &&
            connections[i].state != DISCONNECTED) {
            return i;
        }
    }
    return -1;
}

bool TNCCommands::sendUAFrame(const String& remoteCall, uint8_t remoteSSID) {
    if (!radio) return false;
    
    // Send UA (Unnumbered Acknowledgment) frame
    String uaFrame = "UA:" + config.myCall;
    if (config.mySSID > 0) {
        uaFrame += "-" + String(config.mySSID);
    }
    uaFrame += ">" + remoteCall;
    if (remoteSSID > 0) {
        uaFrame += "-" + String(remoteSSID);
    }
    uaFrame += ":CONNECT_ACKNOWLEDGED:" + String(millis());
    
    if (radio->transmit(uaFrame)) {
        stats.packetsTransmitted++;
        stats.bytesTransmitted += uaFrame.length();
        return true;
    } else {
        stats.packetErrors++;
        return false;
    }
}

bool TNCCommands::isInConverseMode() const {
    // Check if we have any active connections
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].state == CONNECTED) {
            return true;
        }
    }
    return false;
}

bool TNCCommands::sendChatMessage(const String& message) {
    if (!radio) return false;
    
    // Send to all connected stations
    bool sent = false;
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].state == CONNECTED) {
            // Send Information frame
            String iframe = "I:" + config.myCall;
            if (config.mySSID > 0) {
                iframe += "-" + String(config.mySSID);
            }
            iframe += ">" + connections[i].remoteCall;
            if (connections[i].remoteSSID > 0) {
                iframe += "-" + String(connections[i].remoteSSID);
            }
            iframe += ":" + message;
            
            if (radio->transmit(iframe)) {
                stats.packetsTransmitted++;
                stats.bytesTransmitted += iframe.length();
                connections[i].lastActivity = millis();
                sent = true;
            } else {
                stats.packetErrors++;
            }
        }
    }
    
    return sent;
}

void TNCCommands::processReceivedPacket(const String& packet, float rssi, float snr) {
    // This method should be called from the main TNC loop when packets are received
    processIncomingFrame(packet, rssi, snr);
}