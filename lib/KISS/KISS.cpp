#include "KISS.h"

KISS::KISS(Stream &serial, size_t rxCap_, size_t /*txCap*/)
    : port(serial), rxCap(rxCap_)
{
    rx = new uint8_t[rxCap];
    bootTime = millis();
}

KISS::~KISS()
{
    delete[] rx;
}

void KISS::pushSerialByte(uint8_t b)
{
    stats.bytesRx++;
    updateStats();
    switch (rxState)
    {
    case WAIT_FEND:
        if (b == FEND)
        {
            rxState = IN_FRAME;
            rxLen = 0;
        }
        break;

    case IN_FRAME:
        if (b == FEND)
        {
            if (rxLen > 0)
            {
                processFrameData();
            }
            resetFrame();
        }
        else if (b == FESC)
        {
            rxState = ESCAPED;
        }
        else
        {
            if (rxLen < rxCap)
            {
                rx[rxLen++] = b;
            }
            else
            {
                stats.errors++;
                resetFrame();
            }
        }
        break;

    case ESCAPED:
        if (b == TFEND)
        {
            if (rxLen < rxCap)
            {
                rx[rxLen++] = FEND;
            }
            else
            {
                resetFrame();
                return;
            }
        }
        else if (b == TFESC)
        {
            if (rxLen < rxCap)
            {
                rx[rxLen++] = FESC;
            }
            else
            {
                resetFrame();
                return;
            }
        }
        else
        {
            stats.errors++;
            resetFrame();
            return;
        }
        rxState = IN_FRAME;
        break;
    }
}

void KISS::processFrameData()
{
    if (rxLen == 0)
        return;

    uint8_t frameType = rx[0] & 0x0F;
    uint8_t port = (rx[0] >> 4) & 0x0F;

    if (frameType == DATA_FRAME)
    {
        stats.framesRx++;
        if (onFrame && rxLen > 1)
        {
            onFrame(rx + 1, rxLen - 1);
        }
    }
    else
    {
        stats.commandsRx++;
        if (onCommand)
        {
            if (rxLen > 1)
            {
                onCommand(frameType, rx + 1, rxLen - 1);
            }
            else
            {
                onCommand(frameType, nullptr, 0);
            }
        }
    }
}

void KISS::resetFrame()
{
    rxState = WAIT_FEND;
    rxLen = 0;
}

static void kissEscapeWrite(Stream &s, uint8_t b)
{
    if (b == 0xC0)
    {
        s.write(0xDB);
        s.write(0xDC);
    }
    else if (b == 0xDB)
    {
        s.write(0xDB);
        s.write(0xDD);
    }
    else
        s.write(b);
}

void KISS::writeFrame(const uint8_t *data, size_t len)
{
    writeFrameTo(port, data, len);
}

void KISS::writeFrameTo(Stream &s, const uint8_t *data, size_t len)
{
    stats.framesTx++;
    stats.bytesTx += len + 3;
    updateStats();

    s.write(FEND);
    kissEscapeWrite(s, DATA_FRAME);
    for (size_t i = 0; i < len; i++)
        kissEscapeWrite(s, data[i]);
    s.write(FEND);
}

void KISS::writeCommand(uint8_t cmd, const uint8_t *data, size_t len)
{
    port.write(FEND);
    kissEscapeWrite(port, cmd);
    if (data && len > 0)
    {
        for (size_t i = 0; i < len; i++)
            kissEscapeWrite(port, data[i]);
    }
    port.write(FEND);
}

void KISS::updateStats()
{
    stats.uptime = (millis() - bootTime) / 1000;
}
