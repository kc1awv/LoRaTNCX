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
    if (cmd == "help" || cmd == "status" || cmd == "clear" || cmd == "reset") {
        return handleSystemCommand(cmd, args);
    }
    else if (cmd == "lora") {
        return handleLoRaCommand(cmd, args);
    }
    else if (cmd == "tnc") {
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
    
#ifdef FREQUENCY_BAND_868
    Serial.println("Frequency Band: 868 MHz (863-928 MHz, includes 915 MHz)");
#elif defined(FREQUENCY_BAND_915)
    Serial.println("Frequency Band: 915 MHz (902-928 MHz)");
#elif defined(FREQUENCY_BAND_433)
    Serial.println("Frequency Band: 433 MHz (430-440 MHz)");
#else
    Serial.println("Frequency Band: 868 MHz (default)");
#endif
    
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
    else if (cmd == "status") {
        handleStatus();
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
    else {
        Serial.println("Unknown LoRa command. Type 'help' for available commands.");
        return false;
    }
    return true;
}

bool CommandProcessor::handleTncCommand(const String& cmd, const String& args) {
    if (args.length() == 0) {
        Serial.println("TNC command requires arguments. Type 'help' for usage.");
        return false;
    }
    
    int argSpaceIndex = args.indexOf(' ');
    String subCmd = (argSpaceIndex == -1) ? args : args.substring(0, argSpaceIndex);
    String subArgs = (argSpaceIndex == -1) ? "" : args.substring(argSpaceIndex + 1);
    subCmd.toLowerCase();
    
    if (subCmd == "status") {
        handleTncStatus();
    }
    else if (subCmd == "config") {
        handleTncConfig();
    }
    else if (subCmd == "kiss") {
        handleTncKiss();
        return false; // Don't print prompt in KISS mode
    }
    else if (subCmd == "beacon") {
        handleTncBeacon(subArgs);
    }
    else if (subCmd == "csma") {
        handleTncCsma(subArgs);
    }
    else if (subCmd == "test") {
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
    Serial.println("TNC Commands:");
    Serial.println("  tnc status     - Show TNC status and statistics");
    Serial.println("  tnc config     - Show TNC configuration");
    Serial.println("  tnc kiss       - Enter KISS mode");
    Serial.println("  tnc beacon <en> <int> <text> - Set beacon (enable, interval_ms, text)");
    Serial.println("  tnc csma <en> [slot] [retries] - Configure CSMA/CD");
    Serial.println("  tnc test       - Send test frame");
}

void CommandProcessor::handleStatus() {
    Serial.println("System Status:");
    Serial.printf("  Uptime: %u seconds\n", millis() / 1000);
    Serial.printf("  Free Heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("  CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash Size: %u bytes\n", ESP.getFlashChipSize());
    Serial.printf("  Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("  Chip Revision: %u\n", ESP.getChipRevision());
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
    }
    else {
        float freq = args.toFloat();
        loraRadio->setFrequency(freq);
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