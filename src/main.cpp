#include <Arduino.h>
#include "HardwareConfig.h"

void setup()
{
    // Initialize serial communication
    Serial.begin(KISS_BAUD_RATE);
    Serial.println("LoRaTNCX - Fresh Start");
    Serial.println("Hardware: Heltec WiFi LoRa 32 V4 (ESP32-S3)");

    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LED_OFF);

    // Initialize power control
    pinMode(POWER_CTRL_PIN, OUTPUT);
    digitalWrite(POWER_CTRL_PIN, POWER_ON);
    
    // Initialize LoRa PA (Power Amplifier) pins
    pinMode(LORA_PA_POWER_PIN, ANALOG);     // PA power control
    pinMode(LORA_PA_EN_PIN, OUTPUT);        // PA enable
    pinMode(LORA_PA_TX_EN_PIN, OUTPUT);     // PA TX enable
    digitalWrite(LORA_PA_EN_PIN, HIGH);     // Enable PA
    digitalWrite(LORA_PA_TX_EN_PIN, HIGH);  // Enable TX

    // Print pin configuration
    Serial.println("\nPin Configuration:");
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

    // Print some key board pin definitions to verify they're working
    Serial.printf("Board pin definitions: LED_BUILTIN=%d, TX=%d, RX=%d\n",
                  LED_BUILTIN, TX, RX);
    Serial.printf("SPI pins: SS=%d, SCK=%d, MOSI=%d, MISO=%d\n",
                  SS, SCK, MOSI, MISO);

    Serial.println("System initialized - ready for LoRa TNC development");
}

void loop()
{
    // Blink status LED to show system is alive
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink > STATUS_LED_BLINK)
    {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? LED_ON : LED_OFF);
        lastBlink = millis();
    }

    delay(10);
}