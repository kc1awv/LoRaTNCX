#include <Arduino.h>
#include "HardwareConfig.h"
#include "LoRaRadio.h"

LoRaRadio loraRadio;

void setup()
{
    // Initialize serial communication
    Serial.begin(KISS_BAUD_RATE);
    while (!Serial) delay(10);
    
    Serial.println("\n=== LoRaTNCX v2.0 - Heltec WiFi LoRa 32 V4 ===");
    Serial.println("LoRa TNC with RadioLib Integration");

    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LED_OFF);

    // Initialize power control
    pinMode(POWER_CTRL_PIN, OUTPUT);
    digitalWrite(POWER_CTRL_PIN, POWER_ON);
    
    // Initialize LoRa radio subsystem
    Serial.println("\nInitializing LoRa radio...");
    if (loraRadio.begin()) {
        Serial.println("✓ LoRa radio initialized successfully!");
        
        // Send a test beacon
        Serial.println("\nSending test beacon...");
        String beacon = "LoRaTNCX v2.0 - System Online";
        int result = loraRadio.transmit(beacon);
        
        if (result == RADIOLIB_ERR_NONE) {
            Serial.println("✓ Test beacon transmitted successfully!");
        } else {
            Serial.printf("✗ Test beacon failed with code: %d\n", result);
        }
        
        // Radio is ready for both TX and RX operations
        Serial.println("\nRadio ready for TX/RX operations");
        
    } else {
        Serial.println("✗ Failed to initialize LoRa radio!");
        Serial.println("Check hardware connections and configuration");
    }

    // Print comprehensive pin configuration
    Serial.println("\n=== Hardware Configuration ===");
    Serial.printf("LoRa SS: %d, RST: %d, DIO0: %d, BUSY: %d\n",
                  LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_BUSY_PIN);
    Serial.printf("LoRa PA Power: %d, PA EN: %d, PA TX EN: %d\n",
                  LORA_PA_POWER_PIN, LORA_PA_EN_PIN, LORA_PA_TX_EN_PIN);
    Serial.printf("OLED SDA: %d, SCL: %d, RST: %d\n",
                  OLED_SDA_PIN, OLED_SCL_PIN, OLED_RST_PIN);
    Serial.printf("Status LED: %d, Power Control: %d\n",
                  STATUS_LED_PIN, POWER_CTRL_PIN);
    Serial.printf("GNSS VCTL: %d, RX: %d, TX: %d, Wake: %d, PPS: %d, RST: %d\n",
                  GNSS_VCTL_PIN, GNSS_RX_PIN, GNSS_TX_PIN, GNSS_WAKE_PIN, GNSS_PPS_PIN, GNSS_RST_PIN);

    Serial.println("\n=== LoRaTNCX Ready for Operation ===");
}

void loop()
{
    // Periodic test transmission every 30 seconds
    static unsigned long lastTest = 0;
    if (millis() - lastTest > 30000) {
        String testMsg = "LoRaTNCX Beacon #" + String(millis() / 1000);
        Serial.printf("\n[TX] Sending: %s\n", testMsg.c_str());
        int result = loraRadio.transmit(testMsg);
        if (result == RADIOLIB_ERR_NONE) {
            Serial.println("[TX] Success!");
        } else {
            Serial.printf("[TX] Failed with code: %d\n", result);
        }
        lastTest = millis();
    }
    
    // Try to receive messages (with timeout)
    uint8_t buffer[256];
    int packetLength = loraRadio.receive(buffer, sizeof(buffer));
    
    if (packetLength > 0) {
        // Successful reception
        buffer[packetLength] = 0;  // Null-terminate
        Serial.printf("\n[RX] Received: %s\n", (char*)buffer);
        Serial.printf("[RX] RSSI: %.1f dBm, SNR: %.1f dB\n", 
                     loraRadio.getLastRSSI(), loraRadio.getLastSNR());
    }
    
    // Blink status LED to show system is alive
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink > STATUS_LED_BLINK)
    {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? LED_ON : LED_OFF);
        lastBlink = millis();
    }

    delay(100);  // Slightly longer delay for receive polling
}