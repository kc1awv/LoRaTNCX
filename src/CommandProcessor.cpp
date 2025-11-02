#include "CommandProcessor.h"
#include "LoRaRadio.h"
#include "LoRaTNC.h"

CommandProcessor::CommandProcessor(LoRaRadio* radio, LoRaTNC* tncInstance) {
    loraRadio = radio;
    tnc = tncInstance;
    
    // Initialize TNC-2 configuration and heard list
    tnc2Config = new TNC2Config();
    heardList = new StationHeard();
    
    // Initialize command aliases
    initializeAliases();
    
    // Begin TNC-2 configuration (load from NVS)
    tnc2Config->begin();
}

bool CommandProcessor::processCommand(const String& input) {
    String command = input;
    command.trim();
    
    if (command.length() == 0) {
        // Empty command, just return
        return true;
    }
    
    // Parse main command and arguments
    String cmd = getCommand(command);
    String args = getArguments(command);
    
    // Resolve command aliases (before converting to lowercase)
    cmd = resolveCommand(cmd);
    cmd.toLowerCase();
    
    // Route to appropriate handler
    // 1. System commands (help, clear, reset, status)
    if (cmd == "help" || cmd == "clear" || cmd == "reset" || cmd == "status") {
        return handleSystemCommand(cmd, args);
    }
    // 2. LoRa commands (lora ...)
    else if (cmd == "lora") {
        return handleLoRaCommand(cmd, args);
    }
    // 3. TNC commands (config, kiss, beacon, csma, test)
    else if (cmd == "config" || cmd == "kiss" || cmd == "beacon" || cmd == "csma" || cmd == "test") {
        return handleTncCommand(cmd, args);
    }
    // 4. TNC-2 commands
    else if (handleTNC2Command(cmd, args)) {
        return true;
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
        return false;
    }
}

void CommandProcessor::printHeader() {
    Serial.println();
    Serial.println("===============================================");
    Serial.println("        LoRaTNCX Serial Console v1.1");
    Serial.println("===============================================");
    Serial.println();
    
    // Print board information
    Serial.println("Board: Heltec WiFi LoRa 32 V3");
    Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    
    // Show current frequency band configuration
    Serial.println("Frequency Band: Runtime configured (use 'lora bands' to see available)");
    
    Serial.println();
    Serial.println("Console ready. Type 'help' for available commands.");
    Serial.println("-----------------------------------------------");
    Serial.print("> ");
}

String CommandProcessor::getCommand(const String& input) {
    int spaceIndex = input.indexOf(' ');
    return (spaceIndex == -1) ? input : input.substring(0, spaceIndex);
}

String CommandProcessor::getArguments(const String& input) {
    int spaceIndex = input.indexOf(' ');
    return (spaceIndex == -1) ? "" : input.substring(spaceIndex + 1);
}

bool CommandProcessor::handleSystemCommand(const String& cmd, const String& args) {
    if (cmd == "help") {
        if (args.equalsIgnoreCase("tnc2")) {
            printTNC2Help();
        } else {
            handleHelp();
        }
    }
    else if (cmd == "clear") {
        handleClear();
        return true; // Don't print prompt, handleClear() already does it
    }
    else if (cmd == "reset") {
        handleReset();
    }
    return true;
}

bool CommandProcessor::handleLoRaCommand(const String& cmd, const String& args) {
    if (args.length() == 0) {
        Serial.println("LoRa command requires arguments. Type 'help' for usage.");
        return false;
    }
    
    int argSpaceIndex = args.indexOf(' ');
    String subCmd = (argSpaceIndex == -1) ? args : args.substring(0, argSpaceIndex);
    String subArgs = (argSpaceIndex == -1) ? "" : args.substring(argSpaceIndex + 1);
    subCmd.toLowerCase();
    
    if (subCmd == "status") {
        handleLoRaStatus();
    }
    else if (subCmd == "config") {
        handleLoRaConfig();
    }
    else if (subCmd == "stats") {
        handleLoRaStats();
    }
    else if (subCmd == "send") {
        handleLoRaSend(subArgs);
    }
    else if (subCmd == "rx") {
        handleLoRaRx();
    }
    else if (subCmd == "freq") {
        handleLoRaFreq(subArgs);
    }
    else if (subCmd == "power") {
        handleLoRaPower(subArgs);
    }
    else if (subCmd == "sf") {
        handleLoRaSf(subArgs);
    }
    else if (subCmd == "bw") {
        handleLoRaBw(subArgs);
    }
    else if (subCmd == "cr") {
        handleLoRaCr(subArgs);
    }
    else if (subCmd == "bands") {
        handleLoRaBands(subArgs);
    }
    else if (subCmd == "band") {
        handleLoRaBand(subArgs);
    }
    else if (subCmd == "save") {
        handleLoRaSave();
    }
    else {
        Serial.println("Unknown LoRa command. Type 'help' for available commands.");
        return false;
    }
    return true;
}

bool CommandProcessor::handleTncCommand(const String& cmd, const String& args) {
    if (cmd == "status") {
        handleTncStatus();
    }
    else if (cmd == "config") {
        handleTncConfig();
    }
    else if (cmd == "kiss") {
        handleTncKiss();
        return false; // Don't print prompt in KISS mode
    }
    else if (cmd == "beacon") {
        handleTncBeacon(args);
    }
    else if (cmd == "csma") {
        handleTncCsma(args);
    }
    else if (cmd == "test") {
        handleTncTest();
    }
    else {
        Serial.println("Unknown TNC command. Type 'help' for available commands.");
        return false;
    }
    return true;
}

void CommandProcessor::handleHelp() {
    Serial.println("Available commands:");
    Serial.println("System Commands:");
    Serial.println("  help           - Show this help message");
    Serial.println("  help tnc2      - Show TNC-2 compatible commands");
    Serial.println("  status         - Show system status");
    Serial.println("  clear          - Clear screen");
    Serial.println("  reset          - Restart the device");
    Serial.println();
    Serial.println("LoRa Commands:");
    Serial.println("  lora status    - Show LoRa radio status");
    Serial.println("  lora config    - Show LoRa configuration");
    Serial.println("  lora stats     - Show LoRa statistics");
    Serial.println("  lora send <msg>- Send LoRa message");
    Serial.println("  lora rx        - Start continuous receive mode");
    Serial.println("  lora freq <mhz>- Set frequency in MHz (e.g., 868.0)");
    Serial.println("  lora power <db>- Set TX power in dBm");
    Serial.println("  lora sf <sf>   - Set spreading factor (7-12)");
    Serial.println("  lora bw <khz>  - Set bandwidth in kHz (125/250/500) or 0/1/2");
    Serial.println("  lora cr <cr>   - Set coding rate (5-8 for 4/5-4/8) or 1-4");
    Serial.println();
    Serial.println("Frequency Band Commands:");
    Serial.println("  lora bands     - Show all available frequency bands");
    Serial.println("  lora bands ism - Show ISM bands (no license required)");
    Serial.println("  lora bands amateur - Show amateur radio bands");
    Serial.println("  lora band      - Show current band configuration");
    Serial.println("  lora band <id> - Select frequency band (e.g., ISM_915)");
    Serial.println("  lora save      - Save current configuration to NVS memory");
    Serial.println();
    Serial.println("TNC Commands:");
    Serial.println("  status         - Show TNC status and statistics");
    Serial.println("  config         - Show TNC configuration");
    Serial.println("  kiss (K)       - Enter KISS mode");
    Serial.println("  beacon <en> <int> <text> - Set beacon (enable, interval_ms, text)");
    Serial.println("  csma <en> [slot] [retries] - Configure CSMA/CD");
    Serial.println("  test           - Send test frame");
    Serial.println();
    Serial.println("TNC-2 Compatible Commands:");
    Serial.println("  MYcall <call>  - Set station callsign (MY)");
    Serial.println("  Monitor ON/OFF - RF monitoring (M)");
    Serial.println("  MHeard         - Show heard stations (MH)");
    Serial.println("  Beacon E <sec> - Beacon control (B)");
    Serial.println("  DISplay        - Show all parameters (D)");
    Serial.println("  (Type 'help tnc2' for complete TNC-2 command list)");
}

void CommandProcessor::handleClear() {
    // Send ANSI escape sequence to clear screen
    Serial.print("\033[2J\033[H");
    printHeader();
}

void CommandProcessor::handleReset() {
    Serial.println("Restarting device...");
    delay(1000);
    ESP.restart();
}

void CommandProcessor::handleLoRaStatus() {
    const char *stateNames[] = {"IDLE", "TX", "RX", "LOWPOWER"};
    Serial.printf("LoRa Status: %s\n", stateNames[loraRadio->getState()]);
}

void CommandProcessor::handleLoRaConfig() {
    loraRadio->printConfiguration();
}

void CommandProcessor::handleLoRaStats() {
    loraRadio->printStatistics();
}

void CommandProcessor::handleLoRaSend(const String& args) {
    if (args.length() == 0) {
        Serial.println("Usage: lora send <message>");
    }
    else {
        Serial.printf("Sending: \"%s\"\n", args.c_str());
        int result = loraRadio->send(args);
        if (result == RADIOLIB_ERR_NONE) {
            Serial.println("Message queued for transmission");
        }
        else {
            Serial.printf("Failed to queue message (error: %d)\n", result);
        }
    }
}

void CommandProcessor::handleLoRaRx() {
    Serial.println("Starting continuous receive mode...");
    loraRadio->startReceive();
}

void CommandProcessor::handleLoRaFreq(const String& args) {
    if (args.length() == 0) {
        Serial.printf("Current frequency: %.1f MHz\n", loraRadio->getFrequency());
        loraRadio->printCurrentBand();
    }
    else {
        float freq = args.toFloat();
        if (loraRadio->setFrequencyWithBand(freq)) {
            Serial.printf("Frequency set to %.3f MHz\n", freq);
        } else {
            Serial.printf("Failed to set frequency to %.3f MHz - check band restrictions\n", freq);
        }
    }
}

void CommandProcessor::handleLoRaPower(const String& args) {
    if (args.length() == 0) {
        Serial.printf("Current TX power: %d dBm\n", loraRadio->getTxPower());
    }
    else {
        int8_t power = args.toInt();
        loraRadio->setTxPower(power);
    }
}

void CommandProcessor::handleLoRaSf(const String& args) {
    if (args.length() == 0) {
        Serial.printf("Current spreading factor: SF%d\n", loraRadio->getConfig().spreadingFactor);
    }
    else {
        uint8_t sf = args.toInt();
        loraRadio->setSpreadingFactor(sf);
    }
}

void CommandProcessor::handleLoRaBw(const String& args) {
    if (args.length() == 0) {
        Serial.printf("Current bandwidth: %.1f kHz\n", loraRadio->getConfig().bandwidth);
    }
    else {
        float bw = args.toFloat();
        // Convert common values: 0=125, 1=250, 2=500
        if (bw == 0) bw = 125.0;
        else if (bw == 1) bw = 250.0;
        else if (bw == 2) bw = 500.0;
        loraRadio->setBandwidth(bw);
    }
}

void CommandProcessor::handleLoRaCr(const String& args) {
    if (args.length() == 0) {
        Serial.printf("Current coding rate: 4/%d\n", loraRadio->getConfig().codingRate);
    }
    else {
        uint8_t cr = args.toInt();
        // Convert from old format (1-4) to new format (5-8)
        if (cr >= 1 && cr <= 4) {
            cr += 4; // 1->5, 2->6, 3->7, 4->8
        }
        loraRadio->setCodingRate(cr);
    }
}

void CommandProcessor::handleTncStatus() {
    if (tnc) {
        tnc->printStatistics();
    }
    else {
        Serial.println("TNC not initialized");
    }
}

void CommandProcessor::handleTncConfig() {
    if (tnc) {
        tnc->printConfiguration();
    }
    else {
        Serial.println("TNC not initialized");
    }
}

void CommandProcessor::handleTncKiss() {
    if (tnc) {
        // TAPR TNC-2 compatible: silent entry to KISS mode
        // No confirmation message should be sent
        tnc->enterKissMode();
    }
    else {
        Serial.println("TNC not initialized");
    }
}

void CommandProcessor::handleTncBeacon(const String& args) {
    if (!tnc) {
        Serial.println("TNC not initialized");
        return;
    }
    
    // Parse beacon arguments: enable interval text
    int space1 = args.indexOf(' ');
    int space2 = args.indexOf(' ', space1 + 1);
    
    if (space1 == -1 || space2 == -1) {
        Serial.println("Usage: tnc beacon <enable> <interval_ms> <text>");
    }
    else {
        bool enable = args.substring(0, space1).toInt() != 0;
        uint32_t interval = args.substring(space1 + 1, space2).toInt();
        String text = args.substring(space2 + 1);
        tnc->setBeacon(enable, interval, text);
    }
}

void CommandProcessor::handleTncCsma(const String& args) {
    if (!tnc) {
        Serial.println("TNC not initialized");
        return;
    }
    
    int space1 = args.indexOf(' ');
    bool enable = args.substring(0, space1 == -1 ? args.length() : space1).toInt() != 0;
    
    if (space1 == -1) {
        // Just enable/disable with defaults
        tnc->enableCSMA(enable);
    }
    else {
        int space2 = args.indexOf(' ', space1 + 1);
        uint16_t slotTime = args.substring(space1 + 1, space2 == -1 ? args.length() : space2).toInt();
        uint8_t retries = (space2 == -1) ? 10 : args.substring(space2 + 1).toInt();
        
        tnc->enableCSMA(enable, slotTime, retries);
    }
}

void CommandProcessor::handleTncTest() {
    if (tnc) {
        tnc->sendTestFrame();
    }
    else {
        Serial.println("TNC not initialized");
    }
}

void CommandProcessor::handleLoRaBands(const String& args) {
    if (args.length() == 0) {
        // Show all available bands
        loraRadio->printAvailableBands();
    } else {
        // Filter by region or license type
        String filter = args;
        filter.toLowerCase();
        
        if (filter == "ism") {
            Serial.println("\n[FreqBand] ISM Bands (No license required):");
            loraRadio->getBandManager()->printAvailableBands();
            // TODO: Add filtering by license type
        } else if (filter == "amateur" || filter == "ham") {
            Serial.println("\n[FreqBand] Amateur Radio Bands (License required):");
            loraRadio->getBandManager()->printAvailableBands();
            // TODO: Add filtering by license type
        } else {
            Serial.printf("Unknown band filter: %s. Use 'ism' or 'amateur'\n", args.c_str());
        }
    }
}

void CommandProcessor::handleLoRaBand(const String& args) {
    if (args.length() == 0) {
        // Show current band
        loraRadio->printCurrentBand();
    } else {
        // Select a specific band
        String bandId = args;
        bandId.toUpperCase();
        
        if (loraRadio->selectBand(bandId)) {
            Serial.printf("Successfully selected band: %s\n", bandId.c_str());
            loraRadio->printCurrentBand();
        } else {
            Serial.printf("Failed to select band: %s\n", bandId.c_str());
            Serial.println("Use 'lora bands' to see available bands");
        }
    }
}

void CommandProcessor::handleLoRaSave() {
    if (loraRadio && loraRadio->getBandManager()) {
        Serial.println("Saving current LoRa configuration to NVS...");
        if (loraRadio->getBandManager()->saveConfiguration()) {
            Serial.println("Configuration saved successfully!");
            Serial.println("Settings will be restored on next boot.");
        } else {
            Serial.println("Failed to save configuration.");
            Serial.println("NVS may not be available or is corrupted.");
        }
    } else {
        Serial.println("Error: LoRa radio or band manager not available");
    }
}

// ============================================================================
// TNC-2 Command System Implementation
// ============================================================================

void CommandProcessor::initializeAliases() {
    // Core TNC-2 command aliases (capital letters show minimum abbreviation)
    commandAliases["MY"] = "mycall";
    commandAliases["MYC"] = "mycall";
    commandAliases["MYCA"] = "mycall";
    commandAliases["MYCAL"] = "mycall";
    
    commandAliases["MYA"] = "myalias";
    commandAliases["MYAL"] = "myalias";
    commandAliases["MYALI"] = "myalias";
    commandAliases["MYALIA"] = "myalias";
    
    commandAliases["B"] = "beacon";
    commandAliases["BE"] = "beacon";
    commandAliases["BEA"] = "beacon";
    commandAliases["BEAC"] = "beacon";
    commandAliases["BEACO"] = "beacon";
    
    commandAliases["BT"] = "btext";
    commandAliases["BTE"] = "btext";
    commandAliases["BTEX"] = "btext";
    
    commandAliases["CT"] = "ctext";
    commandAliases["CTE"] = "ctext";
    commandAliases["CTEX"] = "ctext";
    
    commandAliases["M"] = "monitor";
    commandAliases["MO"] = "monitor";
    commandAliases["MON"] = "monitor";
    commandAliases["MONI"] = "monitor";
    commandAliases["MONIT"] = "monitor";
    commandAliases["MONITO"] = "monitor";
    
    commandAliases["MH"] = "mheard";
    commandAliases["MHE"] = "mheard";
    commandAliases["MHEA"] = "mheard";
    commandAliases["MHEAR"] = "mheard";
    
    commandAliases["MS"] = "mstamp";
    commandAliases["MST"] = "mstamp";
    commandAliases["MSTA"] = "mstamp";
    commandAliases["MSTAM"] = "mstamp";
    
    commandAliases["C"] = "connect";
    commandAliases["CO"] = "connect";
    commandAliases["CON"] = "connect";
    commandAliases["CONN"] = "connect";
    commandAliases["CONNE"] = "connect";
    commandAliases["CONNEC"] = "connect";
    
    commandAliases["CONO"] = "conok";
    commandAliases["CONOK"] = "conok";
    
    commandAliases["CS"] = "cstatus";
    commandAliases["CST"] = "cstatus";
    commandAliases["CSTA"] = "cstatus";
    commandAliases["CSTAT"] = "cstatus";
    commandAliases["CSTATU"] = "cstatus";
    
    commandAliases["CONV"] = "convers";
    commandAliases["CONVE"] = "convers";
    commandAliases["CONVER"] = "convers";
    
    commandAliases["E"] = "echo";
    commandAliases["EC"] = "echo";
    commandAliases["ECH"] = "echo";
    
    commandAliases["XF"] = "xflow";
    commandAliases["XFL"] = "xflow";
    commandAliases["XFLO"] = "xflow";
    
    commandAliases["DIG"] = "digipeat";
    commandAliases["DIGI"] = "digipeat";
    commandAliases["DIGIP"] = "digipeat";
    commandAliases["DIGIPE"] = "digipeat";
    commandAliases["DIGIPEA"] = "digipeat";
    
    commandAliases["MAX"] = "maxframe";
    commandAliases["MAXF"] = "maxframe";
    commandAliases["MAXFR"] = "maxframe";
    commandAliases["MAXFRA"] = "maxframe";
    commandAliases["MAXFRAM"] = "maxframe";
    
    commandAliases["R"] = "retry";
    commandAliases["RE"] = "retry";
    commandAliases["RET"] = "retry";
    commandAliases["RETR"] = "retry";
    
    commandAliases["P"] = "paclen";
    commandAliases["PA"] = "paclen";
    commandAliases["PAC"] = "paclen";
    commandAliases["PACL"] = "paclen";
    commandAliases["PACLE"] = "paclen";
    
    commandAliases["FR"] = "frack";
    commandAliases["FRA"] = "frack";
    commandAliases["FRAC"] = "frack";
    
    commandAliases["RESP"] = "resptime";
    commandAliases["RESPT"] = "resptime";
    commandAliases["RESPTI"] = "resptime";
    commandAliases["RESPTIM"] = "resptime";
    
    commandAliases["D"] = "display";
    commandAliases["DI"] = "display";
    commandAliases["DIS"] = "display";
    commandAliases["DISP"] = "display";
    commandAliases["DISPL"] = "display";
    commandAliases["DISPLA"] = "display";
    
    // LoRa-enhanced commands  
    commandAliases["RS"] = "rssi";
    commandAliases["RSS"] = "rssi";
    
    commandAliases["SN"] = "snr";
    
    commandAliases["LQ"] = "linkqual";
    commandAliases["LINK"] = "linkqual";
    commandAliases["LINKQ"] = "linkqual";
    commandAliases["LINKQU"] = "linkqual";
    commandAliases["LINKQUA"] = "linkqual";
    
    // Original KISS shorthand
    commandAliases["K"] = "kiss";
}

String CommandProcessor::resolveCommand(const String& input) {
    String upperInput = input;
    upperInput.toUpperCase();
    
    auto it = commandAliases.find(upperInput);
    if (it != commandAliases.end()) {
        return it->second;
    }
    return input;  // Return original if no alias found
}

bool CommandProcessor::handleTNC2Command(const String& cmd, const String& args) {
    // TNC-2 core commands
    if (cmd == "mycall") {
        handleMycall(args);
        return true;
    } else if (cmd == "myalias") {
        handleMyAlias(args);
        return true;
    } else if (cmd == "btext") {
        handleBtext(args);
        return true;
    } else if (cmd == "ctext") {
        handleCtext(args);
        return true;
    } else if (cmd == "beacon") {
        handleBeaconTNC2(args);
        return true;
    } else if (cmd == "monitor") {
        handleMonitorTNC2(args);
        return true;
    } else if (cmd == "mheard") {
        handleMheard(args);
        return true;
    } else if (cmd == "mstamp") {
        handleMstamp(args);
        return true;
    } else if (cmd == "connect") {
        handleConnect(args);
        return true;
    } else if (cmd == "conok") {
        handleConok(args);
        return true;
    } else if (cmd == "cstatus") {
        handleCstatus(args);
        return true;
    } else if (cmd == "convers") {
        handleConvers(args);
        return true;
    } else if (cmd == "maxframe") {
        handleMaxframe(args);
        return true;
    } else if (cmd == "retry") {
        handleRetry(args);
        return true;
    } else if (cmd == "paclen") {
        handlePaclen(args);
        return true;
    } else if (cmd == "frack") {
        handleFrack(args);
        return true;
    } else if (cmd == "resptime") {
        handleResptime(args);
        return true;
    } else if (cmd == "echo") {
        handleEcho(args);
        return true;
    } else if (cmd == "xflow") {
        handleXflow(args);
        return true;
    } else if (cmd == "digipeat") {
        handleDigipeat(args);
        return true;
    } else if (cmd == "display") {
        handleDisplay(args);
        return true;
    } else if (cmd == "rssi") {
        handleRssi(args);
        return true;
    } else if (cmd == "snr") {
        handleSnr(args);
        return true;
    } else if (cmd == "linkqual") {
        handleLinkqual(args);
        return true;
    }
    
    return false; // Command not recognized
}

// ============================================================================
// TNC-2 Command Implementations
// ============================================================================

void CommandProcessor::handleMycall(const String& args) {
    if (args.length() == 0) {
        // Show current callsign
        Serial.printf("MYcall %s\n", tnc2Config->getMyCall().c_str());
        return;
    }
    
    // Set new callsign
    String callsign = args;
    callsign.trim();
    
    if (tnc2Config->setMyCall(callsign)) {
        Serial.printf("MYcall %s\n", tnc2Config->getMyCall().c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid callsign format");
        Serial.println("Format: CALL or CALL-SSID (3-9 characters)");
    }
}

void CommandProcessor::handleMyAlias(const String& args) {
    if (args.length() == 0) {
        // Show current alias
        String alias = tnc2Config->getMyAlias();
        Serial.printf("MYAlias %s\n", alias.isEmpty() ? "(none)" : alias.c_str());
        return;
    }
    
    // Set new alias
    String alias = args;
    alias.trim();
    tnc2Config->setMyAlias(alias);
    Serial.printf("MYAlias %s\n", tnc2Config->getMyAlias().c_str());
    tnc2Config->saveToNVS();
}

void CommandProcessor::handleBtext(const String& args) {
    if (args.length() == 0) {
        // Show current beacon text
        String text = tnc2Config->getBeaconText();
        Serial.printf("BText %s\n", text.isEmpty() ? "(none)" : text.c_str());
        return;
    }
    
    // Set new beacon text
    tnc2Config->setBeaconText(args);
    Serial.printf("BText %s\n", tnc2Config->getBeaconText().c_str());
    tnc2Config->saveToNVS();
}

void CommandProcessor::handleCtext(const String& args) {
    if (args.length() == 0) {
        // Show current connect text
        String text = tnc2Config->getConnectText();
        Serial.printf("CText %s\n", text.isEmpty() ? "(none)" : text.c_str());
        return;
    }
    
    // Set new connect text
    tnc2Config->setConnectText(args);
    Serial.printf("CText %s\n", tnc2Config->getConnectText().c_str());
    tnc2Config->saveToNVS();
}

void CommandProcessor::handleBeaconTNC2(const String& args) {
    if (args.length() == 0) {
        // Show current beacon status
        if (tnc2Config->getBeaconEnabled()) {
            Serial.printf("Beacon EVERY %d\n", tnc2Config->getBeaconInterval());
        } else {
            Serial.println("Beacon OFF");
        }
        return;
    }
    
    // Parse beacon command: E/A/OFF interval
    String argsCopy = args;
    argsCopy.trim();
    argsCopy.toUpperCase();
    
    if (argsCopy == "OFF") {
        tnc2Config->setBeaconEnabled(false);
        Serial.println("Beacon OFF");
        tnc2Config->saveToNVS();
        return;
    }
    
    int spaceIndex = argsCopy.indexOf(' ');
    if (spaceIndex == -1) {
        Serial.println("Usage: Beacon E/A <seconds> or Beacon OFF");
        return;
    }
    
    String mode = argsCopy.substring(0, spaceIndex);
    int interval = argsCopy.substring(spaceIndex + 1).toInt();
    
    if (mode == "E" || mode == "EVERY") {
        if (interval >= 30 && interval <= 86400) { // 30 seconds to 24 hours
            tnc2Config->setBeaconEnabled(true);
            tnc2Config->setBeaconInterval(interval);
            Serial.printf("Beacon EVERY %d\n", interval);
            tnc2Config->saveToNVS();
        } else {
            Serial.println("Invalid interval (30-86400 seconds)");
        }
    } else if (mode == "A" || mode == "AFTER") {
        // After mode - beacon after period of inactivity
        Serial.println("AFTER mode not yet implemented - using EVERY");
        if (interval >= 30 && interval <= 86400) {
            tnc2Config->setBeaconEnabled(true);
            tnc2Config->setBeaconInterval(interval);
            Serial.printf("Beacon EVERY %d\n", interval);
            tnc2Config->saveToNVS();
        } else {
            Serial.println("Invalid interval (30-86400 seconds)");
        }
    } else {
        Serial.println("Usage: Beacon E/A <seconds> or Beacon OFF");
    }
}

void CommandProcessor::handleMonitorTNC2(const String& args) {
    if (args.length() == 0) {
        // Show current monitor status
        Serial.printf("Monitor %s\n", formatOnOff(tnc2Config->getMonitorEnabled()).c_str());
        return;
    }
    
    bool enabled;
    if (parseOnOff(args, enabled)) {
        tnc2Config->setMonitorEnabled(enabled);
        Serial.printf("Monitor %s\n", formatOnOff(enabled).c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Usage: Monitor ON/OFF");
    }
}

void CommandProcessor::handleMheard(const String& args) {
    if (args.length() > 0) {
        String cmd = args;
        cmd.trim();
        cmd.toUpperCase();
        
        if (cmd == "CLEAR") {
            heardList->clearAll();
            Serial.println("Heard list cleared");
            return;
        } else if (cmd.startsWith("FILTER ")) {
            String pattern = cmd.substring(7);
            heardList->printFilteredList(pattern);
            return;
        }
    }
    
    // Show heard list with timestamp if enabled
    heardList->printHeardList(tnc2Config->getTimestampEnabled(), true);
}

void CommandProcessor::handleMstamp(const String& args) {
    if (args.length() == 0) {
        // Show current timestamp status
        Serial.printf("MStamp %s\n", formatOnOff(tnc2Config->getTimestampEnabled()).c_str());
        return;
    }
    
    bool enabled;
    if (parseOnOff(args, enabled)) {
        tnc2Config->setTimestampEnabled(enabled);
        Serial.printf("MStamp %s\n", formatOnOff(enabled).c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Usage: MStamp ON/OFF");
    }
}

void CommandProcessor::handleConnect(const String& args) {
    if (args.length() == 0) {
        Serial.println("Usage: Connect <callsign> [via <path>]");
        return;
    }
    
    // For now, just show that connection is not yet implemented
    Serial.printf("Connect to %s - Not yet implemented\n", args.c_str());
    Serial.println("Connection management will be added in future update");
}

void CommandProcessor::handleConok(const String& args) {
    if (args.length() == 0) {
        // Show current connection OK status
        Serial.printf("CONOk %s\n", formatOnOff(tnc2Config->getConnectionOk()).c_str());
        return;
    }
    
    bool enabled;
    if (parseOnOff(args, enabled)) {
        tnc2Config->setConnectionOk(enabled);
        Serial.printf("CONOk %s\n", formatOnOff(enabled).c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Usage: CONOk ON/OFF");
    }
}

void CommandProcessor::handleCstatus(const String& args) {
    Serial.println("Connection Status:");
    Serial.println("Stream 0: Disconnected");
    Serial.println("Stream 1: Disconnected");
    Serial.println("Stream 2: Disconnected");
    Serial.println("Stream 3: Disconnected");
    Serial.println("Connection management will be added in future update");
}

void CommandProcessor::handleConvers(const String& args) {
    Serial.println("Converse mode not yet implemented");
    Serial.println("This will allow direct conversation through established connections");
}

void CommandProcessor::handleMaxframe(const String& args) {
    if (args.length() == 0) {
        // Show current max frame
        Serial.printf("MAXframe %d\n", tnc2Config->getMaxFrame());
        return;
    }
    
    int frames = args.toInt();
    if (frames >= 1 && frames <= 7) {
        tnc2Config->setMaxFrame(frames);
        Serial.printf("MAXframe %d\n", frames);
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid frame count (1-7)");
    }
}

void CommandProcessor::handleRetry(const String& args) {
    if (args.length() == 0) {
        // Show current retry count
        Serial.printf("RETry %d\n", tnc2Config->getRetryCount());
        return;
    }
    
    int retries = args.toInt();
    if (retries >= 0 && retries <= 15) {
        tnc2Config->setRetryCount(retries);
        Serial.printf("RETry %d\n", retries);
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid retry count (0-15)");
    }
}

void CommandProcessor::handlePaclen(const String& args) {
    if (args.length() == 0) {
        // Show current packet length
        Serial.printf("Paclen %d\n", tnc2Config->getPacketLength());
        return;
    }
    
    int length = args.toInt();
    if (length >= 1 && length <= 255) {
        tnc2Config->setPacketLength(length);
        Serial.printf("Paclen %d\n", length);
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid packet length (1-255)");
    }
}

void CommandProcessor::handleFrack(const String& args) {
    if (args.length() == 0) {
        // Show current frack time
        Serial.printf("FRack %d\n", tnc2Config->getFrackTime());
        return;
    }
    
    int time = args.toInt();
    if (time >= 1 && time <= 250) {
        tnc2Config->setFrackTime(time);
        Serial.printf("FRack %d\n", time);
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid frack time (1-250)");
    }
}

void CommandProcessor::handleResptime(const String& args) {
    if (args.length() == 0) {
        // Show current response time
        Serial.printf("RESptime %d\n", tnc2Config->getRespTime());
        return;
    }
    
    int time = args.toInt();
    if (time >= 0 && time <= 250) {
        tnc2Config->setRespTime(time);
        Serial.printf("RESptime %d\n", time);
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid response time (0-250)");
    }
}

void CommandProcessor::handleEcho(const String& args) {
    if (args.length() == 0) {
        // Show current echo status
        Serial.printf("Echo %s\n", formatOnOff(tnc2Config->getEchoEnabled()).c_str());
        return;
    }
    
    bool enabled;
    if (parseOnOff(args, enabled)) {
        tnc2Config->setEchoEnabled(enabled);
        Serial.printf("Echo %s\n", formatOnOff(enabled).c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Usage: Echo ON/OFF");
    }
}

void CommandProcessor::handleXflow(const String& args) {
    if (args.length() == 0) {
        // Show current xflow status
        Serial.printf("Xflow %s\n", formatOnOff(tnc2Config->getXflowEnabled()).c_str());
        return;
    }
    
    bool enabled;
    if (parseOnOff(args, enabled)) {
        tnc2Config->setXflowEnabled(enabled);
        Serial.printf("Xflow %s\n", formatOnOff(enabled).c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Usage: Xflow ON/OFF");
    }
}

void CommandProcessor::handleDigipeat(const String& args) {
    if (args.length() == 0) {
        // Show current digipeat status
        Serial.printf("DIGipeat %s\n", formatOnOff(tnc2Config->getDigipeatEnabled()).c_str());
        return;
    }
    
    bool enabled;
    if (parseOnOff(args, enabled)) {
        tnc2Config->setDigipeatEnabled(enabled);
        Serial.printf("DIGipeat %s\n", formatOnOff(enabled).c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Usage: DIGipeat ON/OFF");
    }
}

void CommandProcessor::handleDisplay(const String& args) {
    if (args.length() == 0) {
        // Show all parameters
        tnc2Config->printConfiguration();
        return;
    }
    
    // Show specific parameter
    tnc2Config->printParameter(args);
}

void CommandProcessor::handleRssi(const String& args) {
    if (heardList->getCount() == 0) {
        Serial.println("No stations heard - no RSSI data available");
        return;
    }
    
    float avgRssi = heardList->getAverageRSSI();
    StationInfo best = heardList->getBestSignalStation();
    
    Serial.println("RSSI Statistics:");
    Serial.printf("Average RSSI: %.1f dBm\n", avgRssi);
    if (!best.callsign.isEmpty()) {
        Serial.printf("Best signal: %s at %d dBm\n", best.callsign.c_str(), best.lastRSSI);
    }
    
    // Show recent RSSI values
    Serial.println("\nRecent RSSI readings:");
    heardList->printCompactList();
}

void CommandProcessor::handleSnr(const String& args) {
    if (heardList->getCount() == 0) {
        Serial.println("No stations heard - no SNR data available");
        return;
    }
    
    float avgSnr = heardList->getAverageSNR();
    
    Serial.println("SNR Statistics:");
    Serial.printf("Average SNR: %.1f dB\n", avgSnr);
    
    // Show recent SNR values  
    Serial.println("\nRecent SNR readings:");
    heardList->printHeardList(false, true);
}

void CommandProcessor::handleLinkqual(const String& args) {
    if (heardList->getCount() == 0) {
        Serial.println("No link quality data available");
        return;
    }
    
    float avgRssi = heardList->getAverageRSSI();
    float avgSnr = heardList->getAverageSNR();
    
    Serial.println("Link Quality Summary:");
    Serial.printf("Average RSSI: %.1f dBm\n", avgRssi);
    Serial.printf("Average SNR:  %.1f dB\n", avgSnr);
    
    // Provide qualitative assessment
    String quality = "Unknown";
    if (avgRssi > -60 && avgSnr > 8) {
        quality = "Excellent";
    } else if (avgRssi > -80 && avgSnr > 5) {
        quality = "Good";
    } else if (avgRssi > -100 && avgSnr > 0) {
        quality = "Fair";
    } else {
        quality = "Poor";
    }
    
    Serial.printf("Overall Quality: %s\n", quality.c_str());
    Serial.printf("Stations Heard: %d\n", heardList->getCount());
}

// ============================================================================
// Utility Methods
// ============================================================================

String CommandProcessor::formatOnOff(bool value) {
    return value ? "ON" : "OFF";
}

bool CommandProcessor::parseOnOff(const String& input, bool& result) {
    String upper = input;
    upper.trim();
    upper.toUpperCase();
    
    if (upper == "ON" || upper == "1" || upper == "TRUE") {
        result = true;
        return true;
    } else if (upper == "OFF" || upper == "0" || upper == "FALSE") {
        result = false;
        return true;
    }
    
    return false;
}

void CommandProcessor::printTNC2Help() {
    Serial.println("TNC-2 Compatible Commands:");
    Serial.println("==========================");
    Serial.println("Station Identity:");
    Serial.println("  MYcall <call>    - Set/show station callsign");
    Serial.println("  MYAlias <alias>  - Set/show digipeater alias");
    Serial.println();
    Serial.println("Beacon Control:");
    Serial.println("  Beacon E <sec>   - Enable beacon every N seconds");
    Serial.println("  Beacon OFF       - Disable beacon");
    Serial.println("  BText <text>     - Set beacon text");
    Serial.println();
    Serial.println("Monitor Functions:");
    Serial.println("  Monitor ON/OFF   - Enable/disable RF monitoring");
    Serial.println("  MHeard           - Show stations heard");
    Serial.println("  MStamp ON/OFF    - Enable/disable timestamps");
    Serial.println();
    Serial.println("Link Parameters:");
    Serial.println("  MAXframe <1-7>   - Set window size");
    Serial.println("  RETry <0-15>     - Set retry count");
    Serial.println("  Paclen <1-255>   - Set packet length");
    Serial.println("  FRack <1-250>    - Set frame ACK time");
    Serial.println("  RESptime <0-250> - Set response time");
    Serial.println();
    Serial.println("Terminal Control:");
    Serial.println("  Echo ON/OFF      - Terminal echo");
    Serial.println("  Xflow ON/OFF     - XON/XOFF flow control");
    Serial.println();
    Serial.println("Configuration:");
    Serial.println("  DISplay [param]  - Show parameters");
    Serial.println("  CONOk ON/OFF     - Allow connections");
    Serial.println("  DIGipeat ON/OFF  - Enable digipeater");
    Serial.println();
    Serial.println("LoRa Enhancements:");
    Serial.println("  RSsi             - Show RSSI statistics");
    Serial.println("  SNr              - Show SNR statistics");
    Serial.println("  LINKqual         - Show link quality");
    Serial.println();
    Serial.println("Note: Commands support abbreviations (capitals show minimum)");
}