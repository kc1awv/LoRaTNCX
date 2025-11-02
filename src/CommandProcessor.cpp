#include "CommandProcessor.h"
#include "LoRaRadio.h"
#include "LoRaTNC.h"

CommandProcessor::CommandProcessor(LoRaRadio* radio, LoRaTNC* tncInstance) {
    loraRadio = radio;
    tnc = tncInstance;
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
    cmd.toLowerCase();
    
    // Route to appropriate handler
    // 1. System commands (help, clear, reset)
    if (cmd == "help" || cmd == "clear" || cmd == "reset") {
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
        handleHelp();
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
    Serial.println();
    Serial.println("TNC Commands:");
    Serial.println("  status         - Show TNC status and statistics");
    Serial.println("  config         - Show TNC configuration");
    Serial.println("  kiss           - Enter KISS mode");
    Serial.println("  beacon <en> <int> <text> - Set beacon (enable, interval_ms, text)");
    Serial.println("  csma <en> [slot] [retries] - Configure CSMA/CD");
    Serial.println("  test           - Send test frame");
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