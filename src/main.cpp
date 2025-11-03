// Simple test firmware for Heltec WiFi LoRa 32 V3/V4
// - Blinks the onboard LED (LED_BUILTIN)
// - Prints a simple status message over Serial (115200)

#ifndef BUILD_LORA_TEST
#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println();
#ifdef WIFI_LoRa_32_V3
  Serial.println("LoRaTNCX test: Board: heltec_wifi_lora_32_V3");
#else
  Serial.println("LoRaTNCX test: Board: heltec_wifi_lora_32_V4 or unknown");
#endif
  Serial.print("LED_BUILTIN pin: ");
  Serial.println(LED_BUILTIN);
  pinMode(LED_BUILTIN, OUTPUT);
}

unsigned long lastMillis = 0;
bool ledState = false;

void loop()
{
  unsigned long now = millis();
  if (now - lastMillis >= 500)
  {
    lastMillis = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    Serial.print("millis: ");
    Serial.print(now);
    Serial.print("  LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  }
}

#endif // BUILD_LORA_TEST
