// This test file defines setup()/loop() so it must be explicitly enabled
// to avoid multiple-definition linker errors with other sketches in `src/`.
// Define BUILD_LORA_TEST in build_flags to include this file in the build.

#ifdef BUILD_LORA_TEST

#include <Arduino.h>
#include "LoRaRadio.h"
#include "LoRaTNCX.h"

// Common LoRa pin mappings from variants
#ifndef LORA_NSS
#define LORA_NSS SS
#endif

#ifndef RST_LoRa
// fall back to typical variant names
#define RST_LoRa 12
#endif

#ifndef BUSY_LoRa
#define BUSY_LoRa 13
#endif

#ifndef DIO0
#define DIO0 14
#endif

// V4 PA pins (use these values when building for V4)
#if defined(ARDUINO_heltec_wifi_lora_32_V4)
#ifndef LORA_PA_EN
#define LORA_PA_EN 36
#endif
#ifndef LORA_PA_TX_EN
#define LORA_PA_TX_EN 37
#endif
#ifndef LORA_PA_POWER
#define LORA_PA_POWER 38
#endif
#else
// Non-V4: no PA pins
#define LORA_PA_EN -1
#define LORA_PA_TX_EN -1
#define LORA_PA_POWER -1
#endif

LoRaRadio radio(LORA_NSS, BUSY_LoRa, DIO0, RST_LoRa, LORA_PA_EN, LORA_PA_TX_EN, LORA_PA_POWER);
LoRaTNCX tnc(Serial, radio);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  Serial.println("LoRa test: initializing radio...");

  float freq = 868.0; // follow HelTec factory test (MHz)
  if (!radio.begin(freq)) {
    Serial.println("Radio init failed");
    return;
  }

  Serial.print("Radio initialized at "); Serial.print(freq); Serial.println(" MHz");

  const char *msg = "LoRaTNCX test";
  int res = radio.send((const uint8_t*)msg, strlen(msg));
  if (res == 0) {
    Serial.println("Transmit OK");
  } else {
    Serial.print("Transmit failed: "); Serial.println(res);
  }

  // start TNC command processor (uses Serial)
  tnc.begin();
}

void loop() {
  // poll the TNC for serial commands and radio RX
  tnc.poll();
  delay(10);
}

#endif // BUILD_LORA_TEST
