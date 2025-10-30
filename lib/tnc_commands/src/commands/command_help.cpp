#include "CommandContext.h"

TNCCommandResult TNCCommands::handleHELP(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("LoRaTNCX - Comprehensive TNC Commands");
        sendResponse("=====================================");
        sendResponse("");
        sendResponse("BASIC COMMANDS:");
        sendResponse("  HELP       - Show this help");
        sendResponse("  STATUS     - Show system status");
        sendResponse("  VERSION    - Show firmware version");
        sendResponse("  MODE       - Show/set operating mode");
        sendResponse("  MYCALL     - Show/set station callsign");
        sendResponse("  KISS       - Enter KISS mode");
        sendResponse("  CMD        - Enter command mode");
        sendResponse("");
        sendResponse("RADIO CONFIGURATION:");
        sendResponse("  FREQ       - Set/show frequency (MHz)");
        sendResponse("  POWER      - Set/show TX power (dBm)");
        sendResponse("  SF         - Set/show spreading factor");
        sendResponse("  BW         - Set/show bandwidth (kHz)");
        sendResponse("  CR         - Set/show coding rate");
        sendResponse("  SYNC       - Set/show sync word");
        sendResponse("");
        sendResponse("NETWORK & ROUTING:");
        sendResponse("  BEACON     - Configure beacon settings");
        sendResponse("  DIGI       - Configure digipeater mode");
        sendResponse("  ROUTE      - Show/manage routing table");
        sendResponse("  NODES      - Show heard stations");
        sendResponse("");
        sendResponse("PROTOCOL TIMING:");
        sendResponse("  TXDELAY    - TX key-up delay (ms)");
        sendResponse("  TXTAIL     - TX tail time (ms)");
        sendResponse("  PERSIST    - CSMA persistence (0-255)");
        sendResponse("  SLOTTIME   - Slot time for CSMA (ms)");
        sendResponse("  RESPTIME   - Response timeout (ms)");
        sendResponse("  MAXFRAME   - Max frames per transmission");
        sendResponse("  FRACK      - Frame acknowledge timeout");
        sendResponse("  RETRY      - Retry attempts (0-15)");
        sendResponse("  Note: LoRa is simplex (half-duplex only)");
        sendResponse("");
        sendResponse("MONITORING:");
        sendResponse("  STATS      - Show packet statistics");
        sendResponse("  RSSI       - Show last RSSI reading");
        sendResponse("  SNR        - Show last SNR reading");
        sendResponse("  LOG        - Show/configure logging");
        sendResponse("");
        sendResponse("CONFIGURATION:");
        sendResponse("  SAVE       - Save settings to flash");
        sendResponse("  SAVED      - Show settings stored in flash");
        sendResponse("  LOAD       - Load settings from flash");
        sendResponse("  RESET      - Reset to defaults");
        sendResponse("  FACTORY    - Factory reset");
        sendResponse("");
        sendResponse("TESTING:");
        sendResponse("  TEST       - Run system tests");
        sendResponse("  CAL        - Calibration functions");
        sendResponse("  DIAG       - System diagnostics");
        sendResponse("  PING       - Send test packets");
        sendResponse("");
        sendResponse("Type HELP <command> for detailed help on specific commands");
    } else {
        String cmd = toUpperCase(args[0]);
        if (cmd == "FREQ") {
            sendResponse("FREQ [frequency] - Set/show operating frequency");
            sendResponse("  frequency: 902-928 MHz (ISM band)");
            sendResponse("  Examples: FREQ 915.0, FREQ 927.5");
        } else if (cmd == "POWER") {
            sendResponse("POWER [power] - Set/show transmit power");
            sendResponse("  power: -9 to 22 dBm");
            sendResponse("  Examples: POWER 10, POWER 20");
        } else if (cmd == "MYCALL") {
            sendResponse("MYCALL [callsign] - Set/show station callsign");
            sendResponse("  callsign: 3-6 character amateur radio callsign");
            sendResponse("  Examples: MYCALL W1AW, MYCALL KJ4ABC");
        } else if (cmd == "SAVED") {
            sendResponse("SAVED - Display configuration saved in flash");
            sendResponse("  Shows station, radio, protocol, and system settings");
        } else {
            sendResponse("No detailed help available for: " + cmd);
        }
    }
    
    return TNCCommandResult::SUCCESS;
}
