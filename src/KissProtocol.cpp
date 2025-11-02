#include "KissProtocol.h"

KissProtocol::KissProtocol()
{
    state = KISS_STATE_IDLE;
    rxBufferPos = 0;
    config.txDelay = 50;
    config.persistence = 63;
    config.slotTime = 10;
    config.txTail = 1;
    config.fullDuplex = false;
    framesReceived = 0;
    framesSent = 0;
    frameErrors = 0;
    onDataFrameCallback = nullptr;
    onCommandCallback = nullptr;
}

void KissProtocol::begin() { Serial.println("[KISS] Started"); }
void KissProtocol::reset()
{
    state = KISS_STATE_IDLE;
    rxBufferPos = 0;
}
void KissProtocol::setTxDelay(uint8_t delay) { config.txDelay = delay; }
void KissProtocol::setPersistence(uint8_t p) { config.persistence = p; }
void KissProtocol::setSlotTime(uint8_t slotTime) { config.slotTime = slotTime; }
void KissProtocol::setTxTail(uint8_t tail) { config.txTail = tail; }
void KissProtocol::setFullDuplex(bool enable) { config.fullDuplex = enable; }
void KissProtocol::processByte(uint8_t byte) {}
void KissProtocol::processCommand(uint8_t command, uint8_t parameter) {}
void KissProtocol::sendDataFrame(uint8_t *data, uint16_t length) {}
void KissProtocol::onDataFrame(void (*callback)(uint8_t *data, uint16_t length)) { onDataFrameCallback = callback; }
void KissProtocol::onCommand(void (*callback)(uint8_t command, uint8_t parameter)) { onCommandCallback = callback; }
void KissProtocol::handleSerialInput() {}
void KissProtocol::sendToSerial(uint8_t *data, uint16_t length) {}
void KissProtocol::printStatistics() { Serial.printf("KISS Stats: RX=%u TX=%u Err=%u\n", framesReceived, framesSent, frameErrors); }
void KissProtocol::printConfiguration() { Serial.printf("KISS Config: TXD=%d P=%d Slot=%d\n", config.txDelay, config.persistence, config.slotTime); }

String KissProtocol::commandToString(uint8_t command)
{
    switch (command)
    {
    case KISS_CMD_DATA:
        return "DATA";
    case KISS_CMD_TXDELAY:
        return "TXDELAY";
    case KISS_CMD_P:
        return "PERSISTENCE";
    case KISS_CMD_SLOTTIME:
        return "SLOTTIME";
    case KISS_CMD_TXTAIL:
        return "TXTAIL";
    case KISS_CMD_FULLDUPLEX:
        return "FULLDUPLEX";
    case KISS_CMD_SETHARDWARE:
        return "SETHARDWARE";
    case KISS_CMD_RETURN:
        return "RETURN";
    default:
        return "UNKNOWN";
    }
}
