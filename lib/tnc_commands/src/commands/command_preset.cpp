#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePRESET(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Available presets:");
        sendResponse("Basic Presets:");
        sendResponse("  HIGH_SPEED     - Fast data, short range (SF6, 500kHz)");
        sendResponse("  BALANCED       - Good balance of speed/range (SF7, 125kHz)");
        sendResponse("  LONG_RANGE     - Maximum range, slower data (SF12, 62.5kHz)");
        sendResponse("  LOW_POWER      - Power-optimized settings (2dBm)");
        sendResponse("Amateur Radio Optimized:");
        sendResponse("  FAST_BALANCED  - High-speed balanced (SF8, 500kHz)");
        sendResponse("  ROBUST_BALANCED- Robust balanced (SF9, 250kHz)");
        sendResponse("  MAX_RANGE      - Maximum range (SF11, 125kHz)");
        sendResponse("Band-Specific:");
        sendResponse("  AMATEUR_70CM   - 70cm band optimized (432.6MHz)");
        sendResponse("  AMATEUR_33CM   - 33cm band optimized (906MHz)");
        sendResponse("  AMATEUR_23CM   - 23cm band optimized (1290MHz)");
        return TNCCommandResult::SUCCESS;
    }
    
    String preset = toUpperCase(args[0]);
    bool success = true;
    String message = "";
    
    if (preset == "HIGH_SPEED") {
        config.spreadingFactor = 7;
        config.bandwidth = 500.0;
        config.codingRate = 5;
        message = "Applied HIGH_SPEED preset (SF7, 500kHz BW, CR 4/5)";
    } else if (preset == "BALANCED") {
        config.spreadingFactor = 8;
        config.bandwidth = 250.0;
        config.codingRate = 5;
        message = "Applied BALANCED preset (SF8, 250kHz BW, CR 4/5)";
    } else if (preset == "LONG_RANGE") {
        config.spreadingFactor = 10;
        config.bandwidth = 125.0;
        config.codingRate = 5;
        message = "Applied LONG_RANGE preset (SF10, 125kHz BW, CR 4/5)";
    } else if (preset == "LOW_POWER") {
        config.spreadingFactor = 8;
        config.bandwidth = 250.0;
        config.codingRate = 5;
        config.txPower = 2;
        message = "Applied LOW_POWER preset (SF8, 250kHz BW, CR 4/5, 2dBm)";
    } else if (preset == "FAST_BALANCED") {
        config.spreadingFactor = 8;
        config.bandwidth = 500.0;
        config.codingRate = 5;
        message = "Applied FAST_BALANCED preset (SF8, 500kHz BW, CR 4/5)";
    } else if (preset == "ROBUST_BALANCED") {
        config.spreadingFactor = 9;
        config.bandwidth = 250.0;
        config.codingRate = 5;
        message = "Applied ROBUST_BALANCED preset (SF9, 250kHz BW, CR 4/5)";
    } else if (preset == "MAX_RANGE") {
        config.spreadingFactor = 11;
        config.bandwidth = 125.0;
        config.codingRate = 6;
        message = "Applied MAX_RANGE preset (SF11, 125kHz BW, CR 4/6)";
    } else if (preset == "AMATEUR_70CM") {
        config.spreadingFactor = 8;
        config.bandwidth = 250.0;
        config.codingRate = 5;
        config.frequency = 432.6;
        config.band = "70CM";
        message = "Applied AMATEUR_70CM preset (432.6MHz, SF8, 250kHz BW, CR 4/5)";
    } else if (preset == "AMATEUR_33CM") {
        config.spreadingFactor = 7;
        config.bandwidth = 500.0;
        config.codingRate = 5;
        config.frequency = 906.0;
        config.band = "33CM";
        message = "Applied AMATEUR_33CM preset (906MHz, SF7, 500kHz BW, CR 4/5)";
    } else if (preset == "AMATEUR_23CM") {
        config.spreadingFactor = 7;
        config.bandwidth = 500.0;
        config.codingRate = 5;
        config.frequency = 1290.0;
        config.band = "23CM";
        message = "Applied AMATEUR_23CM preset (1290MHz, SF7, 500kHz BW, CR 4/5)";
    } else {
        sendResponse("ERROR: Unknown preset. Use PRESET without arguments to see available options.");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    // Apply settings to radio hardware if available
    if (radio != nullptr) {
        bool radioSuccess = true;
        radioSuccess &= radio->setFrequency(config.frequency);
        radioSuccess &= radio->setSpreadingFactor(config.spreadingFactor);
        radioSuccess &= radio->setBandwidth(config.bandwidth);
        radioSuccess &= radio->setCodingRate(config.codingRate);
        radioSuccess &= radio->setTxPower(config.txPower);
        
        if (!radioSuccess) {
            sendResponse("WARNING: Preset applied to config but some radio settings failed");
        }
    }
    
    sendResponse(message);
    return TNCCommandResult::SUCCESS;
}
