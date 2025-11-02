#pragma once

#include <Arduino.h>
#include "LoRaRadio.h"
#include "KissProtocol.h"

typedef enum
{
    TNC_MODE_COMMAND,    // Command line mode (current mode)
    TNC_MODE_KISS,       // KISS TNC mode
    TNC_MODE_TRANSPARENT // Transparent mode
} tnc_mode_t;

typedef struct
{
    uint32_t packetsFromHost;    // Packets received from host (KISS)
    uint32_t packetsToHost;      // Packets sent to host (KISS)
    uint32_t packetsTransmitted; // Packets transmitted over LoRa
    uint32_t packetsReceived;    // Packets received over LoRa
    uint32_t transmitErrors;     // LoRa transmission errors
    uint32_t receiveErrors;      // LoRa reception errors
    uint32_t kissErrors;         // KISS protocol errors
} tnc_statistics_t;

class LoRaTNC
{
private:
    LoRaRadio *loraRadio;
    KissProtocol *kissProtocol;
    tnc_mode_t currentMode;
    tnc_statistics_t stats;

    // Configuration
    bool beaconEnabled;
    uint32_t beaconInterval; // ms
    unsigned long lastBeaconTime;
    String beaconText;

    // CSMA/CD parameters
    bool csmaEnabled;
    uint16_t csmaSlotTime; // ms
    uint8_t csmaMaxRetries;

    // Internal methods
    void handleKissDataFrame(uint8_t *data, uint16_t length);
    void handleKissCommand(uint8_t command, uint8_t parameter);
    void handleKissLoRaCommand(uint8_t command, uint8_t *data, uint16_t length);

    // CSMA/CD implementation
    bool channelIsBusy();
    bool waitForChannel();
    void performCSMA();

public:
    LoRaTNC(LoRaRadio *lora);
    ~LoRaTNC();

    // Initialization
    bool begin();
    void reset();

    // Mode control
    void setMode(tnc_mode_t mode);
    tnc_mode_t getMode() const { return currentMode; }
    bool enterKissMode();
    bool exitKissMode();

    // Configuration
    void setBeacon(bool enabled, uint32_t intervalMs, const String &text);
    void enableCSMA(bool enable, uint16_t slotTimeMs = 100, uint8_t maxRetries = 10);

    // Statistics
    tnc_statistics_t getStatistics() const { return stats; }
    void printStatistics();
    void clearStatistics();

    // Main processing loop
    void handle();

    // LoRa event handlers (public so they can be called from main.cpp callbacks)
    void handleLoRaReceive(uint8_t *payload, uint16_t size, int16_t rssi, float snr);
    void handleLoRaTransmitDone();
    void handleLoRaTransmitTimeout();

    // Get underlying objects
    LoRaRadio *getLoRaRadio() { return loraRadio; }
    KissProtocol *getKissProtocol() { return kissProtocol; }

    // Utility functions
    void sendTestFrame();
    bool isConnected();
    void printConfiguration();

    // Static callback wrappers for C-style callbacks
    static LoRaTNC *instance;
    static void kissDataFrameWrapper(uint8_t *data, uint16_t length);
    static void kissCommandWrapper(uint8_t command, uint8_t parameter);
    static void kissLoRaCommandWrapper(uint8_t command, uint8_t *data, uint16_t length);
};