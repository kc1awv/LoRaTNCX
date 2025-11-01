#include <Arduino.h>

void printHeader() {
    Serial.println();
    Serial.println("===============================================");
    Serial.println("        LoRaTNCX Serial Console v1.0");
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
    
    Serial.println();
    Serial.println("Console ready. Type commands below:");
    Serial.println("(Echo mode active - input will be echoed back)");
    Serial.println("-----------------------------------------------");
    Serial.print("> ");
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    
    printHeader();
}

String inputBuffer = "";

void processCommand(String command) {
    command.trim();
    
    if (command.length() == 0) {
        // Empty command, just show prompt
        Serial.print("> ");
        return;
    }
    
    // Echo back the received command
    Serial.print("Echo: ");
    Serial.println(command);
    
    // Simple command processing (can be expanded later)
    if (command.equalsIgnoreCase("help")) {
        Serial.println("Available commands:");
        Serial.println("  help    - Show this help message");
        Serial.println("  status  - Show system status");
        Serial.println("  clear   - Clear screen");
        Serial.println("  reset   - Restart the device");
    }
    else if (command.equalsIgnoreCase("status")) {
        Serial.println("System Status:");
        Serial.print("  Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
        Serial.print("  Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
    }
    else if (command.equalsIgnoreCase("clear")) {
        // Send ANSI escape sequence to clear screen
        Serial.print("\033[2J\033[H");
        printHeader();
        return; // Don't print prompt, printHeader() already does it
    }
    else if (command.equalsIgnoreCase("reset")) {
        Serial.println("Restarting device...");
        delay(1000);
        ESP.restart();
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
    
    // Print prompt for next command
    Serial.print("> ");
}

void loop() {
    // Check if data is available on serial port
    if (Serial.available() > 0) {
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
