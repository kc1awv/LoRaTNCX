#include <Arduino.h>
#include "LoRaRadio.h"
#include "LoRaTNC.h"

// Global instances
LoRaRadio loraRadio;
LoRaTNC* tnc;

// LoRa event handlers
void onTxDone() {
    if (tnc) {
        tnc->handleLoRaTransmitDone();
    }
}

void onTxTimeout() {
    if (tnc) {
        tnc->handleLoRaTransmitTimeout();
    }
}

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, float snr) {
    // Forward to TNC for processing
    if (tnc) {
        tnc->handleLoRaReceive(payload, size, rssi, snr);
    }
}

void onRxTimeout() {
    Serial.println("[LoRa] RX timeout occurred");
}

void onRxError() {
    Serial.println("[LoRa] RX error occurred");
}

void printHeader() {
    Serial.println();
    Serial.println("===============================================");
    Serial.println("        LoRaTNCX Serial Console v1.1");
    Serial.println("===============================================");
    Serial.println();
    Serial.print("Board: ");
    
    #ifdef ARDUINO_heltec_wifi_lora_32_V3
        Serial.println("Heltec WiFi LoRa 32 V3");
    #elif defined(ARDUINO_heltec_wifi_lora_32_V4)
        Serial.println("Heltec WiFi LoRa 32 V4");
    #else
        Serial.println("Unknown Board");
    #endif
    
    Serial.print("CPU Frequency: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    
    Serial.print("Frequency Band: ");
    #ifdef FREQ_BAND_433
        Serial.println("433 MHz (433-510 MHz)");
    #elif defined(FREQ_BAND_868)
        Serial.println("868 MHz (863-928 MHz, includes 915 MHz)");
    #else
        Serial.println("Default (868 MHz band)");
    #endif
    
    Serial.println();
    Serial.println("Console ready. Type 'help' for available commands.");
    Serial.println("-----------------------------------------------");
    Serial.print("> ");
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    
    printHeader();
    
    // Initialize LoRa radio
    Serial.println("Initializing LoRa radio...");
    
    // Set up LoRa event handlers
    loraRadio.onTxDone(onTxDone);
    loraRadio.onTxTimeout(onTxTimeout);
    loraRadio.onRxDone(onRxDone);
    loraRadio.onRxTimeout(onRxTimeout);
    loraRadio.onRxError(onRxError);
    
    // Initialize LoRa radio
    if (loraRadio.begin()) {
        Serial.println("LoRa radio initialized successfully!");
        
        // Initialize TNC
        tnc = new LoRaTNC(&loraRadio);
        if (tnc->begin()) {
            Serial.println("TNC initialized successfully!");
        } else {
            Serial.println("Failed to initialize TNC!");
        }
        
        // Start in receive mode
        loraRadio.startReceive();
    } else {
        Serial.println("Failed to initialize LoRa radio!");
    }
    
    Serial.print("> ");
}

String inputBuffer = "";

void processCommand(String command) {
    command.trim();
    
    if (command.length() == 0) {
        // Empty command, just show prompt
        Serial.print("> ");
        return;
    }
    
    // Parse command and arguments
    int spaceIndex = command.indexOf(' ');
    String cmd = (spaceIndex == -1) ? command : command.substring(0, spaceIndex);
    String args = (spaceIndex == -1) ? "" : command.substring(spaceIndex + 1);
    cmd.toLowerCase();
    
    // Command processing
    if (cmd == "help") {
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
    else if (cmd == "status") {
        Serial.println("System Status:");
        Serial.printf("  Uptime: %u seconds\n", millis() / 1000);
        Serial.printf("  Free Heap: %u bytes\n", ESP.getFreeHeap());
        Serial.printf("  CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
        Serial.printf("  Flash Size: %u bytes\n", ESP.getFlashChipSize());
        Serial.printf("  Chip Model: %s\n", ESP.getChipModel());
        Serial.printf("  Chip Revision: %u\n", ESP.getChipRevision());
    }
    else if (cmd == "clear") {
        // Send ANSI escape sequence to clear screen
        Serial.print("\033[2J\033[H");
        printHeader();
        return; // Don't print prompt, printHeader() already does it
    }
    else if (cmd == "reset") {
        Serial.println("Restarting device...");
        delay(1000);
        ESP.restart();
    }
    else if (cmd == "lora") {
        if (args.length() == 0) {
            Serial.println("LoRa command requires arguments. Type 'help' for usage.");
        }
        else {
            int argSpaceIndex = args.indexOf(' ');
            String subCmd = (argSpaceIndex == -1) ? args : args.substring(0, argSpaceIndex);
            String subArgs = (argSpaceIndex == -1) ? "" : args.substring(argSpaceIndex + 1);
            subCmd.toLowerCase();
            
            if (subCmd == "status") {
                const char* stateNames[] = {"IDLE", "TX", "RX", "LOWPOWER"};
                Serial.printf("LoRa Status: %s\n", stateNames[loraRadio.getState()]);
            }
            else if (subCmd == "config") {
                loraRadio.printConfiguration();
            }
            else if (subCmd == "stats") {
                loraRadio.printStatistics();
            }
            else if (subCmd == "send") {
                if (subArgs.length() == 0) {
                    Serial.println("Usage: lora send <message>");
                } else {
                    Serial.printf("Sending: \"%s\"\n", subArgs.c_str());
                    int result = loraRadio.send(subArgs);
                    if (result == RADIOLIB_ERR_NONE) {
                        Serial.println("Message queued for transmission");
                    } else {
                        Serial.printf("Failed to queue message (error: %d)\n", result);
                    }
                }
            }
            else if (subCmd == "rx") {
                Serial.println("Starting continuous receive mode...");
                loraRadio.startReceive();
            }
            else if (subCmd == "freq") {
                if (subArgs.length() == 0) {
                    Serial.printf("Current frequency: %.1f MHz\n", loraRadio.getFrequency());
                } else {
                    float freq = subArgs.toFloat();
                    loraRadio.setFrequency(freq);
                }
            }
            else if (subCmd == "power") {
                if (subArgs.length() == 0) {
                    Serial.printf("Current TX power: %d dBm\n", loraRadio.getTxPower());
                } else {
                    int8_t power = subArgs.toInt();
                    loraRadio.setTxPower(power);
                }
            }
            else if (subCmd == "sf") {
                if (subArgs.length() == 0) {
                    Serial.printf("Current spreading factor: SF%d\n", loraRadio.getConfig().spreadingFactor);
                } else {
                    uint8_t sf = subArgs.toInt();
                    loraRadio.setSpreadingFactor(sf);
                }
            }
            else if (subCmd == "bw") {
                if (subArgs.length() == 0) {
                    Serial.printf("Current bandwidth: %.1f kHz\n", loraRadio.getConfig().bandwidth);
                } else {
                    float bw = subArgs.toFloat();
                    // Convert common values: 0=125, 1=250, 2=500
                    if (bw == 0) bw = 125.0;
                    else if (bw == 1) bw = 250.0;
                    else if (bw == 2) bw = 500.0;
                    loraRadio.setBandwidth(bw);
                }
            }
            else if (subCmd == "cr") {
                if (subArgs.length() == 0) {
                    Serial.printf("Current coding rate: 4/%d\n", loraRadio.getConfig().codingRate);
                } else {
                    uint8_t cr = subArgs.toInt();
                    // Convert from old format (1-4) to new format (5-8)
                    if (cr >= 1 && cr <= 4) {
                        cr += 4; // 1->5, 2->6, 3->7, 4->8
                    }
                    loraRadio.setCodingRate(cr);
                }
            }
            else {
                Serial.println("Unknown LoRa command. Type 'help' for available commands.");
            }
        }
    }
    else if (cmd == "tnc") {
        if (args.length() == 0) {
            Serial.println("TNC command requires arguments. Type 'help' for usage.");
        }
        else {
            int argSpaceIndex = args.indexOf(' ');
            String subCmd = (argSpaceIndex == -1) ? args : args.substring(0, argSpaceIndex);
            String subArgs = (argSpaceIndex == -1) ? "" : args.substring(argSpaceIndex + 1);
            subCmd.toLowerCase();
            
            if (subCmd == "status") {
                if (tnc) {
                    tnc->printStatistics();
                } else {
                    Serial.println("TNC not initialized");
                }
            }
            else if (subCmd == "config") {
                if (tnc) {
                    tnc->printConfiguration();
                } else {
                    Serial.println("TNC not initialized");
                }
            }
            else if (subCmd == "kiss") {
                if (tnc) {
                    tnc->enterKissMode();
                    return; // Don't print prompt in KISS mode
                } else {
                    Serial.println("TNC not initialized");
                }
            }
            else if (subCmd == "beacon") {
                if (tnc) {
                    // Parse beacon arguments: enable interval text
                    int space1 = subArgs.indexOf(' ');
                    int space2 = subArgs.indexOf(' ', space1 + 1);
                    
                    if (space1 == -1 || space2 == -1) {
                        Serial.println("Usage: tnc beacon <enable> <interval_ms> <text>");
                    } else {
                        bool enable = subArgs.substring(0, space1).toInt() != 0;
                        uint32_t interval = subArgs.substring(space1 + 1, space2).toInt();
                        String text = subArgs.substring(space2 + 1);
                        tnc->setBeacon(enable, interval, text);
                    }
                } else {
                    Serial.println("TNC not initialized");
                }
            }
            else if (subCmd == "csma") {
                if (tnc) {
                    int space1 = subArgs.indexOf(' ');
                    bool enable = subArgs.substring(0, space1 == -1 ? subArgs.length() : space1).toInt() != 0;
                    
                    if (space1 == -1) {
                        // Just enable/disable with defaults
                        tnc->enableCSMA(enable);
                    } else {
                        int space2 = subArgs.indexOf(' ', space1 + 1);
                        uint16_t slotTime = subArgs.substring(space1 + 1, space2 == -1 ? subArgs.length() : space2).toInt();
                        uint8_t retries = (space2 == -1) ? 10 : subArgs.substring(space2 + 1).toInt();
                        
                        tnc->enableCSMA(enable, slotTime, retries);
                    }
                } else {
                    Serial.println("TNC not initialized");
                }
            }
            else if (subCmd == "test") {
                if (tnc) {
                    tnc->sendTestFrame();
                } else {
                    Serial.println("TNC not initialized");
                }
            }
            else {
                Serial.println("Unknown TNC command. Type 'help' for available commands.");
            }
        }
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
    
    // Print prompt for next command
    Serial.print("> ");
}

void loop() {
    // Handle LoRa radio events
    loraRadio.handle();
    
    // Handle TNC processing
    if (tnc) {
        tnc->handle();
    }
    
    // Check if data is available on serial port (only in command mode)
    if (tnc && tnc->getMode() == TNC_MODE_COMMAND && Serial.available() > 0) {
        char incomingChar = Serial.read();
        
        // Handle different characters
        if (incomingChar == '\r' || incomingChar == '\n') {
            // End of line - process the command
            // Only process if we have content or this is the first line ending character
            if (inputBuffer.length() > 0 || incomingChar == '\r') {
                if (incomingChar == '\r') {
                    // Echo the carriage return as newline for proper formatting
                    Serial.println();
                }
                
                processCommand(inputBuffer);
                inputBuffer = ""; // Clear the buffer for next command
            }
            // Ignore \n if it immediately follows \r (CRLF sequence)
        }
        else if (incomingChar == '\b' || incomingChar == 127) {
            // Backspace or DEL - remove last character
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                // Echo backspace sequence to terminal
                Serial.print("\b \b");
            }
        }
        else if (incomingChar >= 32 && incomingChar < 127) {
            // Printable character - add to buffer and echo
            inputBuffer += incomingChar;
            Serial.print(incomingChar); // Echo the character back
        }
        // Ignore other control characters
    }
}
