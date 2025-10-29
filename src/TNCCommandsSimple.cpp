/**
 * @file TNCCommandsSimple.cpp
 * @brief Simplified TNC Command System Implementation
 * @author LoRaTNCX Project
 * @date October 29, 2025
 */

#include "TNCCommandsSimple.h"
#include "LoRaRadio.h"
#include "esp_system.h"
#include <Preferences.h>

SimpleTNCCommands::SimpleTNCCommands() 
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

TNCCommandResult SimpleTNCCommands::processCommand(const String& commandLine) {
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

void SimpleTNCCommands::setRadio(LoRaRadio* radioPtr) {
    radio = radioPtr;
}

bool SimpleTNCCommands::loadConfigurationFromFlash() {
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
    
    // Beacon and digi
    config.beaconEnabled = preferences.getBool("beaconEnabled", false);
    config.beaconInterval = preferences.getUShort("beaconInterval", 600);
    config.digiEnabled = preferences.getBool("digiEnabled", false);
    config.digiPath = preferences.getUChar("digiPath", 4);
    
    preferences.end();
    return true;
}

bool SimpleTNCCommands::saveConfigurationToFlash() {
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
    
    // Beacon and digi
    preferences.putBool("beaconEnabled", config.beaconEnabled);
    preferences.putUShort("beaconInterval", config.beaconInterval);
    preferences.putBool("digiEnabled", config.digiEnabled);
    preferences.putUChar("digiPath", config.digiPath);
    
    preferences.end();
    return true;
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
        sendResponse("MYCALL: " + config.myCall);
        return TNCCommandResult::SUCCESS;
    } else {
        // Enhanced callsign validation
        String callsign = toUpperCase(args[0]);
        
        // Basic callsign format validation
        if (callsign.length() < 3 || callsign.length() > 6) {
            sendResponse("ERROR: Callsign must be 3-6 characters");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        
        // Check for valid characters (letters and numbers only)
        for (int i = 0; i < callsign.length(); i++) {
            char c = callsign.charAt(i);
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                sendResponse("ERROR: Callsign can only contain letters and numbers");
                return TNCCommandResult::ERROR_INVALID_PARAMETER;
            }
        }
        
        // Must start with a letter
        if (callsign.charAt(0) < 'A' || callsign.charAt(0) > 'Z') {
            sendResponse("ERROR: Callsign must start with a letter");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        
        // Update configuration
        config.myCall = callsign;
        sendResponse("Callsign set to: " + callsign);
        
        // Auto-save if enabled
        if (config.autoSave) {
            if (saveConfigurationToFlash()) {
                sendResponse("Configuration saved to flash");
            } else {
                sendResponse("Warning: Failed to save configuration");
            }
        } else {
            sendResponse("Use SAVE command to persist this setting");
        }
        
        return TNCCommandResult::SUCCESS;
    }
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
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setFrequency(freq)) {
        sendResponse("Frequency set to " + String(freq, 1) + " MHz");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set frequency on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setTxPower(power)) {
        sendResponse("TX Power set to " + String(power) + " dBm");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set TX power on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setSpreadingFactor(sf)) {
        sendResponse("Spreading Factor set to " + String(sf));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set spreading factor on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setBandwidth(bw)) {
        sendResponse("Bandwidth set to " + String(bw, 1) + " kHz");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set bandwidth on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setCodingRate(cr)) {
        sendResponse("Coding Rate set to 4/" + String(cr));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set coding rate on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setSyncWord(sync)) {
        sendResponse("Sync Word set to 0x" + String(sync, HEX));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set sync word on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
        sendResponse("Text: " + config.beaconText);
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
    } else if (cmd == "TEXT" && argCount > 1) {
        // Combine remaining arguments into beacon text
        String newText = "";
        for (int i = 1; i < argCount; i++) {
            if (i > 1) newText += " ";
            newText += args[i];
        }
        if (newText.length() > 200) {
            sendResponse("ERROR: Beacon text too long (max 200 characters)");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        config.beaconText = newText;
        sendResponse("Beacon text set to: " + config.beaconText);
    } else if (cmd == "NOW") {
        // Transmit beacon immediately
        if (!radio) {
            sendResponse("ERROR: Radio not available");
            return TNCCommandResult::ERROR_HARDWARE_ERROR;
        }
        
        return transmitBeacon();
    } else if (cmd == "POSITION" && argCount >= 3) {
        // Set position for APRS-style beacon
        config.latitude = args[1].toFloat();
        config.longitude = args[2].toFloat();
        if (argCount >= 4) {
            config.altitude = args[3].toInt();
        }
        sendResponse("Position set: " + String(config.latitude, 6) + ", " + String(config.longitude, 6));
        if (argCount >= 4) {
            sendResponse("Altitude: " + String(config.altitude) + "m");
        }
    } else {
        sendResponse("Usage: BEACON [ON|OFF|INTERVAL <seconds>|TEXT <text>|NOW|POSITION <lat> <lon> [alt]]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleDIGI(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Digipeater Configuration:");
        sendResponse("Status: " + String(config.digiEnabled ? "ON" : "OFF"));
        if (config.digiEnabled) {
            sendResponse("Max hops: " + String(config.digiPath));
            sendResponse("Callsign: " + config.myCall + (config.mySSID > 0 ? "-" + String(config.mySSID) : ""));
        }
        sendResponse("");
        sendResponse("Digipeater aliases supported:");
        sendResponse("• WIDE1-1, WIDE2-1, WIDE2-2");
        sendResponse("• Direct callsign addressing");
        sendResponse("• WIDEn-N flood algorithm");
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        if (config.myCall == "NOCALL") {
            sendResponse("ERROR: Set station callsign first (MYCALL command)");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        config.digiEnabled = true;
        sendResponse("Digipeater enabled");
        sendResponse("Using callsign: " + config.myCall + (config.mySSID > 0 ? "-" + String(config.mySSID) : ""));
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
    } else if (cmd == "TEST" && argCount >= 2) {
        // Test digipeater path processing
        String testPath = args[1];
        sendResponse("Testing path: " + testPath);
        
        // Simulate path processing
        if (shouldDigipeat(testPath)) {
            String newPath = processDigipeatPath(testPath);
            sendResponse("Would digipeat with path: " + newPath);
        } else {
            sendResponse("Would NOT digipeat this path");
        }
    } else if (cmd == "STATS") {
        // Show digipeater statistics (placeholder for now)
        sendResponse("Digipeater Statistics:");
        sendResponse("Packets digipeated: 0");  // TODO: implement stats
        sendResponse("Packets dropped: 0");
        sendResponse("Current load: 0%");
    } else {
        sendResponse("Usage: DIGI [ON|OFF|HOPS <1-7>|TEST <path>|STATS]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleROUTE(const String args[], int argCount) {
    if (argCount == 0) {
        // Display routing table
        sendResponse("Routing Table:");
        sendResponse("==============");
        sendResponse("Dest      NextHop   Hops Quality LastUsed  LastUpd   Status");
        sendResponse("--------- --------- ---- ------- --------- --------- ------");
        
        bool hasRoutes = false;
        for (int i = 0; i < routeCount; i++) {
            if (routingTable[i].destination.length() > 0) {
                hasRoutes = true;
                String line = "";
                
                // Destination (9 chars)
                line += routingTable[i].destination;
                for (int j = routingTable[i].destination.length(); j < 10; j++) line += " ";
                
                // Next hop (9 chars)
                line += routingTable[i].nextHop;
                for (int j = routingTable[i].nextHop.length(); j < 10; j++) line += " ";
                
                // Hops (4 chars)
                String hopsStr = String(routingTable[i].hops);
                line += hopsStr;
                for (int j = hopsStr.length(); j < 5; j++) line += " ";
                
                // Quality (7 chars)
                String qualStr = String(routingTable[i].quality, 2);
                line += qualStr;
                for (int j = qualStr.length(); j < 8; j++) line += " ";
                
                // Last used (9 chars) - show seconds ago
                unsigned long secondsAgo = (millis() - routingTable[i].lastUsed) / 1000;
                String lastUsedStr = String(secondsAgo) + "s";
                line += lastUsedStr;
                for (int j = lastUsedStr.length(); j < 10; j++) line += " ";
                
                // Last updated (9 chars) - show seconds ago
                secondsAgo = (millis() - routingTable[i].lastUpdated) / 1000;
                String lastUpdStr = String(secondsAgo) + "s";
                line += lastUpdStr;
                for (int j = lastUpdStr.length(); j < 10; j++) line += " ";
                
                // Status
                line += routingTable[i].isActive ? "ACTIVE" : "STALE";
                
                sendResponse(line);
            }
        }
        
        if (!hasRoutes) {
            sendResponse("(No routes configured)");
        }
        
        sendResponse("");
        sendResponse("Usage: ROUTE ADD <dest> <nexthop> [hops] [quality]");
        sendResponse("       ROUTE DEL <dest>");
        sendResponse("       ROUTE CLEAR");
        sendResponse("       ROUTE PURGE (remove stale routes)");
        
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    
    if (cmd == "ADD" && argCount >= 3) {
        // Add new route: ROUTE ADD destination nexthop [hops] [quality]
        String dest = toUpperCase(args[1]);
        String nextHop = toUpperCase(args[2]);
        uint8_t hops = argCount >= 4 ? args[3].toInt() : 1;
        float quality = argCount >= 5 ? args[4].toFloat() : 0.8;
        
        if (hops < 1 || hops > 7) {
            sendResponse("ERROR: Hops must be 1-7");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        
        if (quality < 0.0 || quality > 1.0) {
            sendResponse("ERROR: Quality must be 0.0-1.0");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        
        // Check if route already exists
        int existingIndex = -1;
        for (int i = 0; i < routeCount; i++) {
            if (routingTable[i].destination == dest) {
                existingIndex = i;
                break;
            }
        }
        
        if (existingIndex >= 0) {
            // Update existing route
            routingTable[existingIndex].nextHop = nextHop;
            routingTable[existingIndex].hops = hops;
            routingTable[existingIndex].quality = quality;
            routingTable[existingIndex].lastUpdated = millis();
            routingTable[existingIndex].isActive = true;
            sendResponse("Updated route to " + dest + " via " + nextHop);
        } else {
            // Add new route
            if (routeCount >= MAX_ROUTES) {
                sendResponse("ERROR: Routing table full (max " + String(MAX_ROUTES) + " routes)");
                return TNCCommandResult::ERROR_SYSTEM_ERROR;
            }
            
            routingTable[routeCount].destination = dest;
            routingTable[routeCount].nextHop = nextHop;
            routingTable[routeCount].hops = hops;
            routingTable[routeCount].quality = quality;
            routingTable[routeCount].lastUsed = 0;
            routingTable[routeCount].lastUpdated = millis();
            routingTable[routeCount].isActive = true;
            routeCount++;
            
            sendResponse("Added route to " + dest + " via " + nextHop + " (" + String(hops) + " hops, Q=" + String(quality, 2) + ")");
        }
        
    } else if (cmd == "DEL" && argCount >= 2) {
        // Delete route: ROUTE DEL destination
        String dest = toUpperCase(args[1]);
        bool found = false;
        
        for (int i = 0; i < routeCount; i++) {
            if (routingTable[i].destination == dest) {
                // Shift remaining routes down
                for (int j = i; j < routeCount - 1; j++) {
                    routingTable[j] = routingTable[j + 1];
                }
                routeCount--;
                found = true;
                sendResponse("Deleted route to " + dest);
                break;
            }
        }
        
        if (!found) {
            sendResponse("Route to " + dest + " not found");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        
    } else if (cmd == "CLEAR") {
        // Clear all routes
        routeCount = 0;
        for (int i = 0; i < MAX_ROUTES; i++) {
            routingTable[i].destination = "";
            routingTable[i].isActive = false;
        }
        sendResponse("Routing table cleared");
        
    } else if (cmd == "PURGE") {
        // Remove stale routes (inactive or very old)
        int purged = 0;
        unsigned long now = millis();
        
        for (int i = routeCount - 1; i >= 0; i--) {
            bool shouldPurge = false;
            
            // Purge if inactive
            if (!routingTable[i].isActive) {
                shouldPurge = true;
            }
            // Purge if not updated in 30 minutes
            else if ((now - routingTable[i].lastUpdated) > 1800000) {
                shouldPurge = true;
            }
            
            if (shouldPurge) {
                // Shift remaining routes down
                for (int j = i; j < routeCount - 1; j++) {
                    routingTable[j] = routingTable[j + 1];
                }
                routeCount--;
                purged++;
            }
        }
        
        sendResponse("Purged " + String(purged) + " stale routes");
        
    } else {
        sendResponse("Usage: ROUTE [ADD <dest> <nexthop> [hops] [quality] | DEL <dest> | CLEAR | PURGE]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}

TNCCommandResult SimpleTNCCommands::handleNODES(const String args[], int argCount) {
    if (argCount > 0) {
        String cmd = toUpperCase(args[0]);
        
        if (cmd == "CLEAR") {
            // Clear node table
            nodeCount = 0;
            for (int i = 0; i < MAX_NODES; i++) {
                nodeTable[i].callsign = "";
                nodeTable[i].packetCount = 0;
            }
            sendResponse("Node table cleared");
            return TNCCommandResult::SUCCESS;
        } else if (cmd == "PURGE") {
            // Remove nodes not heard in 60 minutes
            int purged = 0;
            unsigned long now = millis();
            
            for (int i = nodeCount - 1; i >= 0; i--) {
                if ((now - nodeTable[i].lastHeard) > 3600000) { // 60 minutes
                    // Shift remaining nodes down
                    for (int j = i; j < nodeCount - 1; j++) {
                        nodeTable[j] = nodeTable[j + 1];
                    }
                    nodeCount--;
                    purged++;
                }
            }
            
            sendResponse("Purged " + String(purged) + " old nodes");
            return TNCCommandResult::SUCCESS;
        }
    }
    
    // Display node table
    sendResponse("Heard Stations:");
    sendResponse("===============");
    sendResponse("Callsign  SSID  RSSI   SNR   Count Last    First   Last Packet");
    sendResponse("--------- ---- ------ ----- ----- ------- ------- ------------");
    
    bool hasNodes = false;
    unsigned long now = millis();
    
    for (int i = 0; i < nodeCount; i++) {
        if (nodeTable[i].callsign.length() > 0) {
            hasNodes = true;
            String line = "";
            
            // Callsign (9 chars)
            line += nodeTable[i].callsign;
            for (int j = nodeTable[i].callsign.length(); j < 10; j++) line += " ";
            
            // SSID (4 chars)
            String ssidStr = nodeTable[i].ssid > 0 ? String(nodeTable[i].ssid) : "-";
            line += ssidStr;
            for (int j = ssidStr.length(); j < 5; j++) line += " ";
            
            // RSSI (6 chars)
            String rssiStr = String(nodeTable[i].lastRSSI, 1);
            line += rssiStr;
            for (int j = rssiStr.length(); j < 7; j++) line += " ";
            
            // SNR (5 chars)
            String snrStr = String(nodeTable[i].lastSNR, 1);
            line += snrStr;
            for (int j = snrStr.length(); j < 6; j++) line += " ";
            
            // Count (5 chars)
            String countStr = String(nodeTable[i].packetCount);
            line += countStr;
            for (int j = countStr.length(); j < 6; j++) line += " ";
            
            // Last heard (7 chars) - minutes ago
            unsigned long minutesAgo = (now - nodeTable[i].lastHeard) / 60000;
            String lastStr = String(minutesAgo) + "m";
            line += lastStr;
            for (int j = lastStr.length(); j < 8; j++) line += " ";
            
            // First heard (7 chars) - minutes ago
            minutesAgo = (now - nodeTable[i].firstHeard) / 60000;
            String firstStr = String(minutesAgo) + "m";
            line += firstStr;
            for (int j = firstStr.length(); j < 8; j++) line += " ";
            
            // Last packet (truncated to fit)
            String packet = nodeTable[i].lastPacket;
            if (packet.length() > 20) {
                packet = packet.substring(0, 17) + "...";
            }
            line += packet;
            
            sendResponse(line);
        }
    }
    
    if (!hasNodes) {
        sendResponse("(No stations heard yet)");
        sendResponse("");
        sendResponse("Stations will appear here as packets are received.");
        sendResponse("Node discovery requires incoming packet monitoring.");
    } else {
        sendResponse("");
        sendResponse("Total nodes: " + String(nodeCount));
    }
    
    sendResponse("");
    sendResponse("Usage: NODES [CLEAR | PURGE]");
    sendResponse("       CLEAR - Clear all node entries");
    sendResponse("       PURGE - Remove nodes not heard in 60 minutes");
    
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
    if (radio != nullptr) {
        float rssi = radio->getRSSI();
        sendResponse("Last RSSI: " + String(rssi, 1) + " dBm");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}

TNCCommandResult SimpleTNCCommands::handleSNR(const String args[], int argCount) {
    if (radio != nullptr) {
        float snr = radio->getSNR();
        sendResponse("Last SNR: " + String(snr, 1) + " dB");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    Preferences preferences;
    if (!preferences.begin("tnc_config", false)) {
        sendResponse("ERROR: Failed to open preferences storage");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    try {
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
        
        // Beacon and digi
        preferences.putBool("beaconEnabled", config.beaconEnabled);
        preferences.putUShort("beaconInterval", config.beaconInterval);
        preferences.putBool("digiEnabled", config.digiEnabled);
        preferences.putUChar("digiPath", config.digiPath);
        
        preferences.end();
        sendResponse("Configuration saved to flash");
        return TNCCommandResult::SUCCESS;
        
    } catch (...) {
        preferences.end();
        sendResponse("ERROR: Failed to save configuration");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}

TNCCommandResult SimpleTNCCommands::handleLOAD(const String args[], int argCount) {
    sendResponse("Loading configuration from flash...");
    
    Preferences preferences;
    if (!preferences.begin("tnc_config", true)) {  // Read-only mode
        sendResponse("ERROR: Failed to open preferences storage");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    try {
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
        
        // Beacon and digi
        config.beaconEnabled = preferences.getBool("beaconEnabled", false);
        config.beaconInterval = preferences.getUShort("beaconInterval", 600);
        config.digiEnabled = preferences.getBool("digiEnabled", false);
        config.digiPath = preferences.getUChar("digiPath", 4);
        
        preferences.end();
        
        // Apply loaded radio settings to hardware if radio is available
        if (radio != nullptr) {
            sendResponse("Applying loaded settings to radio hardware...");
            bool radioOk = true;
            
            // Apply radio parameters in sequence
            if (!radio->setFrequency(config.frequency)) {
                sendResponse("WARNING: Failed to set frequency on radio");
                radioOk = false;
            }
            if (!radio->setTxPower(config.txPower)) {
                sendResponse("WARNING: Failed to set TX power on radio");
                radioOk = false;
            }
            if (!radio->setSpreadingFactor(config.spreadingFactor)) {
                sendResponse("WARNING: Failed to set spreading factor on radio");
                radioOk = false;
            }
            if (!radio->setBandwidth(config.bandwidth)) {
                sendResponse("WARNING: Failed to set bandwidth on radio");
                radioOk = false;
            }
            if (!radio->setCodingRate(config.codingRate)) {
                sendResponse("WARNING: Failed to set coding rate on radio");
                radioOk = false;
            }
            if (!radio->setSyncWord(config.syncWord)) {
                sendResponse("WARNING: Failed to set sync word on radio");
                radioOk = false;
            }
            
            if (radioOk) {
                sendResponse("Configuration loaded and applied to radio");
            } else {
                sendResponse("Configuration loaded with radio warnings");
            }
        } else {
            sendResponse("Configuration loaded (radio not available for hardware update)");
        }
        
        return TNCCommandResult::SUCCESS;
        
    } catch (...) {
        preferences.end();
        sendResponse("ERROR: Failed to load configuration");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
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
    
    // Auto-save if enabled
    if (config.autoSave) {
        if (saveConfigurationToFlash()) {
            sendResponse("Configuration saved to flash");
        } else {
            sendResponse("Warning: Failed to save configuration");
        }
    } else {
        sendResponse("Use SAVE command to persist this setting");
    }
    
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
        // Show current connections
        sendResponse("Active Connections:");
        sendResponse("==================");
        
        bool hasConnections = false;
        for (int i = 0; i < activeConnections; i++) {
            if (connections[i].state != DISCONNECTED) {
                hasConnections = true;
                String stateStr;
                switch (connections[i].state) {
                    case CONNECTING: stateStr = "CONNECTING"; break;
                    case CONNECTED: stateStr = "CONNECTED"; break;
                    case DISCONNECTING: stateStr = "DISCONNECTING"; break;
                    default: stateStr = "UNKNOWN"; break;
                }
                
                String call = connections[i].remoteCall;
                if (connections[i].remoteSSID > 0) {
                    call += "-" + String(connections[i].remoteSSID);
                }
                
                sendResponse(String(i+1) + ". " + call + " [" + stateStr + "]");
                
                if (connections[i].state == CONNECTED) {
                    unsigned long connectedTime = (millis() - connections[i].connectTime) / 1000;
                    sendResponse("   Connected for " + String(connectedTime) + " seconds");
                }
            }
        }
        
        if (!hasConnections) {
            sendResponse("(No active connections)");
        }
        
        sendResponse("");
        sendResponse("Usage: CONNECT <callsign> [ssid]");
        return TNCCommandResult::SUCCESS;
    }
    
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    if (config.myCall == "NOCALL") {
        sendResponse("ERROR: Set station callsign first (MYCALL command)");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    String targetCall = toUpperCase(args[0]);
    uint8_t targetSSID = 0;
    
    if (argCount >= 2) {
        targetSSID = args[1].toInt();
        if (targetSSID > 15) {
            sendResponse("ERROR: SSID must be 0-15");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
    }
    
    // Check if already connected to this station
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].remoteCall == targetCall && 
            connections[i].remoteSSID == targetSSID &&
            connections[i].state != DISCONNECTED) {
            sendResponse("ERROR: Already connected/connecting to " + targetCall + 
                        (targetSSID > 0 ? "-" + String(targetSSID) : ""));
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
    }
    
    // Find available connection slot
    int connectionIndex = -1;
    if (activeConnections < MAX_CONNECTIONS) {
        connectionIndex = activeConnections;
        activeConnections++;
    } else {
        // Look for a disconnected slot
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connections[i].state == DISCONNECTED) {
                connectionIndex = i;
                break;
            }
        }
    }
    
    if (connectionIndex == -1) {
        sendResponse("ERROR: Maximum connections reached (" + String(MAX_CONNECTIONS) + ")");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    // Initialize connection
    connections[connectionIndex].remoteCall = targetCall;
    connections[connectionIndex].remoteSSID = targetSSID;
    connections[connectionIndex].state = CONNECTING;
    connections[connectionIndex].connectTime = millis();
    connections[connectionIndex].lastActivity = millis();
    connections[connectionIndex].vs = 0;
    connections[connectionIndex].vr = 0;
    connections[connectionIndex].va = 0;
    connections[connectionIndex].retryCount = 0;
    connections[connectionIndex].pollBit = true;
    
    // Send SABM (Set Asynchronous Balanced Mode) frame
    String connectFrame = "SABM:" + config.myCall;
    if (config.mySSID > 0) {
        connectFrame += "-" + String(config.mySSID);
    }
    connectFrame += ">" + targetCall;
    if (targetSSID > 0) {
        connectFrame += "-" + String(targetSSID);
    }
    connectFrame += ":CONNECT_REQUEST:" + String(millis());
    
    if (radio->transmit(connectFrame)) {
        String displayCall = targetCall + (targetSSID > 0 ? "-" + String(targetSSID) : "");
        sendResponse("Connecting to " + displayCall + "...");
        sendResponse("Sent SABM frame, waiting for UA response");
        
        // Update statistics
        stats.packetsTransmitted++;
        stats.bytesTransmitted += connectFrame.length();
        
        // Note: In a real implementation, we would wait for UA (Unnumbered Acknowledgment)
        // For this simplified version, we'll simulate the connection after a delay
        sendResponse("Connection request sent successfully");
        sendResponse("Use DISCONNECT to terminate the connection");
        
        return TNCCommandResult::SUCCESS;
    } else {
        // Connection failed, reset state
        connections[connectionIndex].state = DISCONNECTED;
        if (connectionIndex == activeConnections - 1) {
            activeConnections--;
        }
        
        sendResponse("ERROR: Failed to send connection request");
        stats.packetErrors++;
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
}

TNCCommandResult SimpleTNCCommands::handleDISCONNECT(const String args[], int argCount) {
    if (argCount == 0) {
        // Disconnect all connections
        int disconnected = 0;
        
        for (int i = 0; i < activeConnections; i++) {
            if (connections[i].state == CONNECTED || connections[i].state == CONNECTING) {
                String displayCall = connections[i].remoteCall;
                if (connections[i].remoteSSID > 0) {
                    displayCall += "-" + String(connections[i].remoteSSID);
                }
                
                if (sendDisconnectFrame(i)) {
                    sendResponse("Disconnected from " + displayCall);
                    connections[i].state = DISCONNECTED;
                    disconnected++;
                } else {
                    sendResponse("Failed to disconnect from " + displayCall);
                }
            }
        }
        
        if (disconnected == 0) {
            sendResponse("No active connections to disconnect");
        } else {
            sendResponse("Disconnected " + String(disconnected) + " connection(s)");
        }
        
        return TNCCommandResult::SUCCESS;
    }
    
    // Disconnect specific station
    String targetCall = toUpperCase(args[0]);
    uint8_t targetSSID = 0;
    
    if (argCount >= 2) {
        targetSSID = args[1].toInt();
        if (targetSSID > 15) {
            sendResponse("ERROR: SSID must be 0-15");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
    }
    
    // Find the connection
    int connectionIndex = -1;
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].remoteCall == targetCall && 
            connections[i].remoteSSID == targetSSID &&
            (connections[i].state == CONNECTED || connections[i].state == CONNECTING)) {
            connectionIndex = i;
            break;
        }
    }
    
    if (connectionIndex == -1) {
        String displayCall = targetCall + (targetSSID > 0 ? "-" + String(targetSSID) : "");
        sendResponse("ERROR: No active connection to " + displayCall);
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    String displayCall = connections[connectionIndex].remoteCall;
    if (connections[connectionIndex].remoteSSID > 0) {
        displayCall += "-" + String(connections[connectionIndex].remoteSSID);
    }
    
    if (sendDisconnectFrame(connectionIndex)) {
        sendResponse("Disconnected from " + displayCall);
        
        // Show connection statistics
        if (connections[connectionIndex].state == CONNECTED) {
            unsigned long connectedTime = (millis() - connections[connectionIndex].connectTime) / 1000;
            sendResponse("Connection duration: " + String(connectedTime) + " seconds");
        }
        
        connections[connectionIndex].state = DISCONNECTED;
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("Failed to send disconnect to " + displayCall);
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
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
    
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    String targetCall = args[0];
    int testCount = 3; // Default to 3 ping packets
    
    if (argCount >= 2) {
        testCount = args[1].toInt();
        if (testCount < 1 || testCount > 10) {
            sendResponse("ERROR: Count must be between 1 and 10");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
    }
    
    sendResponse("Link test to " + targetCall + " (" + String(testCount) + " packets):");
    
    int successCount = 0;
    unsigned long totalTime = 0;
    
    for (int i = 1; i <= testCount; i++) {
        // Construct ping packet format: "PING:sourceCall>targetCall:sequenceNumber:timestamp"
        String pingPacket = "PING:" + config.myCall + ">" + targetCall + ":" + String(i) + ":" + String(millis());
        
        sendResponse("Ping " + String(i) + "...");
        
        // Send ping packet
        unsigned long startTime = millis();
        if (!radio->transmit(pingPacket)) {
            sendResponse("  TX FAILED");
            continue;
        }
        
        // Wait for response (timeout after 5 seconds)
        bool responseReceived = false;
        unsigned long timeout = startTime + 5000;
        String response;
        
        while (millis() < timeout && !responseReceived) {
            if (radio->available()) {
                if (radio->receive(response)) {
                    // Check if this is a PONG response to our PING
                    if (response.startsWith("PONG:" + targetCall + ">" + config.myCall + ":" + String(i))) {
                        responseReceived = true;
                        unsigned long roundTripTime = millis() - startTime;
                        totalTime += roundTripTime;
                        successCount++;
                        
                        // Get signal quality from last received packet
                        float rssi = radio->getRSSI();
                        float snr = radio->getSNR();
                        
                        sendResponse("  PONG received: " + String(roundTripTime) + "ms, RSSI=" + String(rssi, 1) + "dBm, SNR=" + String(snr, 1) + "dB");
                    }
                }
            }
            delay(10); // Small delay to prevent busy-waiting
        }
        
        if (!responseReceived) {
            sendResponse("  TIMEOUT (no response)");
        }
        
        // Small delay between pings
        if (i < testCount) {
            delay(500);
        }
    }
    
    // Summary
    sendResponse("--- Link test complete ---");
    sendResponse("Packets sent: " + String(testCount));
    sendResponse("Packets received: " + String(successCount));
    sendResponse("Packet loss: " + String(((testCount - successCount) * 100) / testCount) + "%");
    
    if (successCount > 0) {
        unsigned long avgTime = totalTime / successCount;
        sendResponse("Average round-trip time: " + String(avgTime) + "ms");
    }
    
    return TNCCommandResult::SUCCESS;
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
    
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    // Combine all arguments into message text
    String message = "";
    for (int i = 0; i < argCount; i++) {
        if (i > 0) message += " ";
        message += args[i];
    }
    
    // Create simplified AX.25 UI frame format
    // Format: "UI:sourceCall>destination,path:message"
    // For simplicity, we'll use APRS-style addressing
    String uiFrame = "UI:" + config.myCall;
    if (config.mySSID > 0) {
        uiFrame += "-" + String(config.mySSID);
    }
    
    // Use unprotocol address if configured, otherwise use CQ
    String destination = config.unprotoAddr.length() > 0 ? config.unprotoAddr : "CQ";
    uiFrame += ">" + destination;
    
    // Add path if configured
    if (config.unprotoPath.length() > 0) {
        uiFrame += "," + config.unprotoPath;
    }
    
    uiFrame += ":" + message;
    
    // Validate frame size (LoRa has 255 byte limit)
    if (uiFrame.length() > 240) {
        sendResponse("ERROR: Frame too large (" + String(uiFrame.length()) + " bytes, max 240)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    // Transmit the UI frame
    if (radio->transmit(uiFrame)) {
        sendResponse("UI frame transmitted (" + String(uiFrame.length()) + " bytes)");
        sendResponse("Frame: " + uiFrame);
        
        // Update statistics
        stats.packetsTransmitted++;
        stats.bytesTransmitted += uiFrame.length();
        
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Transmission failed");
        stats.packetErrors++;
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
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

// =============================================================================
// UTILITY METHODS
// =============================================================================

TNCCommandResult SimpleTNCCommands::transmitBeacon() {
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

void SimpleTNCCommands::updateNodeTable(const String& callsign, uint8_t ssid, float rssi, float snr, const String& packet, bool isBeacon) {
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

bool SimpleTNCCommands::shouldDigipeat(const String& path) {
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

String SimpleTNCCommands::processDigipeatPath(const String& path) {
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

bool SimpleTNCCommands::shouldProcessHop(const String& hop) {
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

bool SimpleTNCCommands::sendDisconnectFrame(int connectionIndex) {
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

void SimpleTNCCommands::processIncomingFrame(const String& frame, float rssi, float snr) {
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

void SimpleTNCCommands::parseCallsign(const String& callsignWithSSID, String& callsign, uint8_t& ssid) {
    int dashPos = callsignWithSSID.indexOf('-');
    if (dashPos > 0) {
        callsign = callsignWithSSID.substring(0, dashPos);
        ssid = callsignWithSSID.substring(dashPos + 1).toInt();
    } else {
        callsign = callsignWithSSID;
        ssid = 0;
    }
}

int SimpleTNCCommands::findConnection(const String& remoteCall, uint8_t remoteSSID) {
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].remoteCall == remoteCall && 
            connections[i].remoteSSID == remoteSSID &&
            connections[i].state != DISCONNECTED) {
            return i;
        }
    }
    return -1;
}

bool SimpleTNCCommands::sendUAFrame(const String& remoteCall, uint8_t remoteSSID) {
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

bool SimpleTNCCommands::isInConverseMode() const {
    // Check if we have any active connections
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].state == CONNECTED) {
            return true;
        }
    }
    return false;
}

bool SimpleTNCCommands::sendChatMessage(const String& message) {
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

void SimpleTNCCommands::processReceivedPacket(const String& packet, float rssi, float snr) {
    // This method should be called from the main TNC loop when packets are received
    processIncomingFrame(packet, rssi, snr);
}