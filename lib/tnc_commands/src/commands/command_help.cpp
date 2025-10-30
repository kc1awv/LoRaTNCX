#include "CommandContext.h"

TNCCommandResult TNCCommands::handleHELP(const String args[], int argCount) {
    struct HelpEntry {
        const char* command;
        const char* description;
        uint8_t category;
    };

    static const char* CATEGORY_NAMES[] = {
        "Basic & Mode Control",
        "Interface Settings",
        "Station Identification & Beaconing",
        "Radio Configuration",
        "Protocol Timing & Link Control",
        "Network & Routing",
        "Monitoring & Telemetry",
        "RF Tools & Analysis",
        "Testing & Diagnostics",
        "Storage & System Management"
    };

    static const HelpEntry HELP_ENTRIES[] = {
        {"HELP",        "Show this help overview", 0},
        {"STATUS",      "Show system status", 0},
        {"VERSION",     "Show firmware version", 0},
        {"MODE",        "Show or set operating mode", 0},
        {"KISS",        "Enter KISS (binary) mode", 0},
        {"CMD",         "Return to command mode", 0},
        {"TERMINAL",    "Switch to terminal/chat mode", 0},
        {"TRANSPARENT", "Switch to transparent connected mode", 0},
        {"SIMPLEX",     "Force simplex channel operation", 0},
        {"CONNECT",     "Initiate a connection", 0},
        {"DISCONNECT",  "Terminate active connections", 0},
        {"QUIT",        "Exit to command mode without disconnect", 0},

        {"PROMPT",      "Enable or disable the command prompt", 1},
        {"ECHO",        "Control local command echo", 1},
        {"LINECR",      "Enable/disable carriage return in responses", 1},
        {"LINELF",      "Enable/disable line feed in responses", 1},

        {"MYCALL",      "Show or set station callsign", 2},
        {"MYSSID",      "Show or set station SSID", 2},
        {"BEACON",      "Configure scheduled beaconing", 2},
        {"BCON",        "Immediate beacon control", 2},
        {"BTEXT",       "Set beacon message text", 2},
        {"ID",          "Control station ID beacons", 2},
        {"CWID",        "Enable or disable CW ID", 2},
        {"LICENSE",     "Set regulatory license class", 2},
        {"LOCATION",    "Set GPS coordinates", 2},
        {"GRID",        "Set Maidenhead grid square", 2},
        {"APRS",        "Enable and configure APRS features", 2},

        {"FREQ",        "Set or show operating frequency", 3},
        {"POWER",       "Set or show transmitter power", 3},
        {"SF",          "Set or show spreading factor", 3},
        {"BW",          "Set or show channel bandwidth", 3},
        {"CR",          "Set or show coding rate", 3},
        {"SYNC",        "Set or show sync word", 3},
        {"PREAMBLE",    "Configure LoRa preamble length", 3},
        {"PACTL",       "Control the PA (power amplifier)", 3},
        {"BAND",        "Select amateur radio band plan", 3},
        {"REGION",      "Select regional compliance profile", 3},
        {"COMPLIANCE",  "Show or set compliance options", 3},
        {"EMERGENCY",   "Toggle emergency operating mode", 3},
        {"SENSITIVITY", "Adjust receiver sensitivity target", 3},

        {"TXDELAY",     "Transmit key-up delay", 4},
        {"TXTAIL",      "Transmit tail timing", 4},
        {"PERSIST",     "CSMA persistence value", 4},
        {"SLOTTIME",    "CSMA slot time", 4},
        {"RESPTIME",    "Response timeout", 4},
        {"MAXFRAME",    "Maximum outstanding frames", 4},
        {"FRACK",       "Frame acknowledge timeout", 4},
        {"RETRY",       "Retry attempts", 4},

        {"DIGI",        "Configure digipeater operation", 5},
        {"ROUTE",       "Show or edit routing table", 5},
        {"NODES",       "List heard stations", 5},
        {"UNPROTO",     "Set unproto destination/path", 5},
        {"UIDWAIT",     "Configure UID wait timer", 5},
        {"UIDFRAME",    "Control UI frame transmission", 5},
        {"MCON",        "Toggle monitor of connected frames", 5},
        {"USERS",       "Set maximum simultaneous users", 5},
        {"FLOW",        "Control flow-control behaviour", 5},

        {"STATS",       "Show packet statistics", 6},
        {"RSSI",        "Show last received RSSI", 6},
        {"SNR",         "Show last received SNR", 6},
        {"LOG",         "Show or configure logging", 6},
        {"MONITOR",     "Enable or disable packet monitor", 6},
        {"MHEARD",      "Show heard-station history", 6},
        {"TEMPERATURE", "Read radio temperature", 6},
        {"VOLTAGE",     "Read supply voltage", 6},
        {"UPTIME",      "Show system uptime", 6},
        {"LORASTAT",    "Display detailed LoRa statistics", 6},

        {"RANGE",       "Estimate link range", 7},
        {"TOA",         "Calculate time-on-air", 7},
        {"LINKTEST",    "Run link testing utility", 7},

        {"TEST",        "Run system tests", 8},
        {"CAL",         "Calibration utilities", 8},
        {"CALIBRATE",   "Detailed calibration routine", 8},
        {"DIAG",        "System diagnostics", 8},
        {"PING",        "Send test packet", 8},
        {"SELFTEST",    "Run self-test suite", 8},
        {"DEBUG",       "Set debug verbosity", 8},
        {"GNSS",        "Control GNSS module", 8},

        {"SAVE",        "Save settings to flash", 9},
        {"SAVED",       "Show settings stored in flash", 9},
        {"LOAD",        "Load settings from flash", 9},
        {"RESET",       "Reset settings to defaults", 9},
        {"FACTORY",     "Perform factory reset", 9},
        {"DEFAULT",     "Restore recommended defaults", 9},
        {"PRESET",      "Apply stored configuration preset", 9},
        {"MEMORY",      "Manage memory profiles", 9}
    };

    const size_t CATEGORY_COUNT = sizeof(CATEGORY_NAMES) / sizeof(CATEGORY_NAMES[0]);
    const size_t HELP_ENTRY_COUNT = sizeof(HELP_ENTRIES) / sizeof(HELP_ENTRIES[0]);

    if (argCount == 0) {
        sendResponse("LoRaTNCX - Comprehensive TNC Command Reference");
        sendResponse("=============================================");
        sendResponse("");

        for (size_t category = 0; category < CATEGORY_COUNT; ++category) {
            sendResponse(String(CATEGORY_NAMES[category]) + ":");
            for (size_t i = 0; i < HELP_ENTRY_COUNT; ++i) {
                if (HELP_ENTRIES[i].category != category) {
                    continue;
                }

                String line = "  ";
                line += HELP_ENTRIES[i].command;
                while (line.length() < 16) {
                    line += ' ';
                }
                line += "- ";
                line += HELP_ENTRIES[i].description;
                sendResponse(line);
            }
            sendResponse("");
        }

        sendResponse("Type HELP <command> for detailed help on a specific command");
    } else {
        String cmd = toUpperCase(args[0]);
        bool handled = false;

        if (cmd == "FREQ") {
            sendResponse("FREQ [frequency] - Set/show operating frequency");
            sendResponse("  frequency: 902-928 MHz (ISM band)");
            sendResponse("  Examples: FREQ 915.0, FREQ 927.5");
            handled = true;
        } else if (cmd == "POWER") {
            sendResponse("POWER [power] - Set/show transmit power");
            sendResponse("  power: -9 to 22 dBm");
            sendResponse("  Examples: POWER 10, POWER 20");
            handled = true;
        } else if (cmd == "MYCALL") {
            sendResponse("MYCALL [callsign] - Set/show station callsign");
            sendResponse("  callsign: 3-6 character amateur radio callsign");
            sendResponse("  Examples: MYCALL W1AW, MYCALL KJ4ABC");
            handled = true;
        } else if (cmd == "SAVED") {
            sendResponse("SAVED - Display configuration saved in flash");
            sendResponse("  Shows station, radio, protocol, and system settings");
            handled = true;
        }

        if (!handled) {
            const HelpEntry* entry = nullptr;
            for (size_t i = 0; i < HELP_ENTRY_COUNT; ++i) {
                if (cmd == HELP_ENTRIES[i].command) {
                    entry = &HELP_ENTRIES[i];
                    break;
                }
            }

            if (entry != nullptr) {
                sendResponse(String(entry->command) + " - " + entry->description);
                sendResponse(String("Category: ") + CATEGORY_NAMES[entry->category]);
            } else {
                sendResponse("No detailed help available for: " + cmd);
            }
        }
    }

    return TNCCommandResult::SUCCESS;
}
