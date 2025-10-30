#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSIMPLEX(const String args[], int argCount) {
    sendResponse("LoRa Radio Operating Mode:");
    sendResponse("==========================");
    sendResponse("Mode: SIMPLEX (Half-Duplex)");
    sendResponse("");
    sendResponse("LoRa radios are inherently simplex devices that can");
    sendResponse("either transmit OR receive, but not both simultaneously.");
    sendResponse("");
    sendResponse("This means:");
    sendResponse("• No full-duplex communication possible");
    sendResponse("• CSMA/collision avoidance is essential");
    sendResponse("• Listen-before-talk protocols required");
    sendResponse("• Turn-around time needed between TX/RX");
    sendResponse("");
    sendResponse("Use TXDELAY, SLOTTIME, and PERSIST parameters");
    sendResponse("to optimize channel access and avoid collisions.");
    return TNCCommandResult::SUCCESS;
}
