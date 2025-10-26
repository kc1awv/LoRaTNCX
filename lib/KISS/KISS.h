#pragma once
#include <Arduino.h>
#include <functional>

class KISS
{
public:
    using FrameCB = std::function<void(const uint8_t *, size_t)>;
    using CommandCB = std::function<void(uint8_t, const uint8_t *, size_t)>;

    KISS(Stream &serial, size_t rxCap, size_t txCap);
    ~KISS();
    void setOnFrame(FrameCB cb) { onFrame = cb; }
    void setOnCommand(CommandCB cb) { onCommand = cb; }

    void pushSerialByte(uint8_t b);
    void writeFrame(const uint8_t *data, size_t len);
    void writeFrameTo(Stream &s, const uint8_t *data, size_t len);
    void writeCommand(uint8_t cmd, const uint8_t *data = nullptr, size_t len = 0);

    struct Stats
    {
        unsigned long framesRx = 0;
        unsigned long framesTx = 0;
        unsigned long commandsRx = 0;
        unsigned long bytesRx = 0;
        unsigned long bytesTx = 0;
        unsigned long errors = 0;
        unsigned long uptime = 0;
    };

    const Stats &getStats() const { return stats; }
    void resetStats()
    {
        stats = Stats{};
        bootTime = millis();
    }

    enum FrameType : uint8_t
    {
        DATA_FRAME = 0x00,
        TX_DELAY = 0x01,
        PERSISTENCE = 0x02,
        SLOT_TIME = 0x03,
        TX_TAIL = 0x04,
        FULL_DUPLEX = 0x05,
        SET_HARDWARE = 0x06,
        RETURN = 0xFF
    };

private:
    Stream &port;
    FrameCB onFrame;
    CommandCB onCommand;
    uint8_t *rx;
    size_t rxLen = 0;
    size_t rxCap;

    enum : uint8_t
    {
        FEND = 0xC0,
        FESC = 0xDB,
        TFEND = 0xDC,
        TFESC = 0xDD
    };

    enum RxState : uint8_t
    {
        WAIT_FEND,
        IN_FRAME,
        ESCAPED
    };

    RxState rxState = WAIT_FEND;
    Stats stats;
    unsigned long bootTime;

    void processFrameData();
    void resetFrame();
    void updateStats();
};