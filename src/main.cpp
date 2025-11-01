#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_BUILTIN, OUTPUT);
    
    Serial.println("=== LoRaTNCX Board Test ===");
    Serial.println("Minimal test program loaded");
    Serial.print("Board: ");
    
    #ifdef ARDUINO_heltec_wifi_lora_32_V3
        Serial.println("Heltec V3");
    #elif defined(ARDUINO_heltec_wifi_lora_32_V4)
        Serial.println("Heltec V4");
    #else
        Serial.println("Unknown");
    #endif
    
    Serial.println("LED blink test starting...");
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED ON");
    delay(1000);
    
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED OFF");
    delay(1000);
}
