#ifndef TNC2CONFIG_H
#define TNC2CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

class TNC2Config {
private:
    Preferences prefs;
    
    // Core TNC-2 parameters
    String myCall;
    String myAlias;
    String beaconText;
    String connectText;
    uint16_t beaconInterval;  // seconds
    bool beaconEnabled;
    
    // Monitor parameters
    bool monitorEnabled;
    bool timestampEnabled;
    bool echoEnabled;
    bool xflowEnabled;
    
    // Link parameters  
    uint8_t maxFrame;
    uint8_t retryCount;
    uint16_t packetLength;
    uint16_t frackTime;     // frame ACK timeout (units * 100ms)
    uint16_t respTime;      // response time (units * 100ms)
    bool connectionOk;
    
    // Digipeater parameters
    bool digipeatEnabled;
    
    // Flow control
    uint8_t xonChar;        // Default Ctrl+Q ($11)
    uint8_t xoffChar;       // Default Ctrl+S ($13)
    uint8_t commandChar;    // Default Ctrl+C ($03)
    
    // Terminal settings
    uint8_t screenWidth;    // Default 80
    uint8_t characterLength; // 7 or 8 bit
    
    // Validation helpers
    bool isValidCallsign(const String& call);
    
public:
    TNC2Config();
    ~TNC2Config();
    
    // Initialization
    void begin();
    void loadDefaults();
    void loadFromNVS();
    void saveToNVS();
    void resetToDefaults();
    
    // Station identification
    bool setMyCall(const String& call);
    String getMyCall() const { return myCall; }
    
    void setMyAlias(const String& alias);
    String getMyAlias() const { return myAlias; }
    
    // Beacon configuration
    void setBeaconText(const String& text);
    String getBeaconText() const { return beaconText; }
    
    void setBeaconEnabled(bool enabled) { beaconEnabled = enabled; }
    bool getBeaconEnabled() const { return beaconEnabled; }
    
    void setBeaconInterval(uint16_t seconds);
    uint16_t getBeaconInterval() const { return beaconInterval; }
    
    // Connect text
    void setConnectText(const String& text);
    String getConnectText() const { return connectText; }
    
    // Monitor configuration
    void setMonitorEnabled(bool enabled) { monitorEnabled = enabled; }
    bool getMonitorEnabled() const { return monitorEnabled; }
    
    void setTimestampEnabled(bool enabled) { timestampEnabled = enabled; }
    bool getTimestampEnabled() const { return timestampEnabled; }
    
    void setEchoEnabled(bool enabled) { echoEnabled = enabled; }
    bool getEchoEnabled() const { return echoEnabled; }
    
    void setXflowEnabled(bool enabled) { xflowEnabled = enabled; }
    bool getXflowEnabled() const { return xflowEnabled; }
    
    // Link parameters
    void setMaxFrame(uint8_t frames);
    uint8_t getMaxFrame() const { return maxFrame; }
    
    void setRetryCount(uint8_t retries);
    uint8_t getRetryCount() const { return retryCount; }
    
    void setPacketLength(uint16_t length);
    uint16_t getPacketLength() const { return packetLength; }
    
    void setFrackTime(uint16_t time);
    uint16_t getFrackTime() const { return frackTime; }
    
    void setRespTime(uint16_t time);
    uint16_t getRespTime() const { return respTime; }
    
    void setConnectionOk(bool ok) { connectionOk = ok; }
    bool getConnectionOk() const { return connectionOk; }
    
    // Digipeater
    void setDigipeatEnabled(bool enabled) { digipeatEnabled = enabled; }
    bool getDigipeatEnabled() const { return digipeatEnabled; }
    
    // Flow control characters
    void setXonChar(uint8_t ch) { xonChar = ch; }
    uint8_t getXonChar() const { return xonChar; }
    
    void setXoffChar(uint8_t ch) { xoffChar = ch; }
    uint8_t getXoffChar() const { return xoffChar; }
    
    void setCommandChar(uint8_t ch) { commandChar = ch; }
    uint8_t getCommandChar() const { return commandChar; }
    
    // Terminal settings
    void setScreenWidth(uint8_t width);
    uint8_t getScreenWidth() const { return screenWidth; }
    
    void setCharacterLength(uint8_t length);
    uint8_t getCharacterLength() const { return characterLength; }
    
    // Display
    void printConfiguration();
    void printParameter(const String& param);
};

#endif