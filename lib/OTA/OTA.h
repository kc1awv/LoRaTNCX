#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

/**
 * @brief OTA (Over-The-Air) Update Manager
 * 
 * Provides comprehensive firmware update capabilities via:
 * - Arduino OTA (for development)
 * - Web-based upload interface
 * - Remote update checking
 * - Rollback protection
 * - Progress monitoring
 */
class OTAManager 
{
public:
    enum class UpdateStatus {
        IDLE,
        CHECKING,
        DOWNLOADING,
        INSTALLING,
        SUCCESS,
        FAILED,
        ROLLBACK
    };

    enum class UpdateSource {
        ARDUINO_OTA,
        WEB_UPLOAD,
        HTTP_DOWNLOAD
    };

    struct UpdateInfo {
        String version;
        String description;
        String downloadUrl;
        String checksum;
        size_t fileSize;
        bool forceUpdate;
        UpdateSource source;
    };

    struct ProgressInfo {
        UpdateStatus status;
        size_t bytesReceived;
        size_t totalBytes;
        uint8_t percentage;
        String statusMessage;
        unsigned long startTime;
        unsigned long estimatedTimeRemaining;
    };

    // Initialization and configuration
    static void begin(const String& hostname = "loratnc", uint16_t port = 3232);
    static void setPassword(const String& password);
    static void setUpdateServer(const String& serverUrl);
    static void setCheckInterval(unsigned long intervalMs);
    
    // Core functionality
    static void handle();
    static void checkForUpdates();
    static bool startUpdate(const UpdateInfo& updateInfo);
    static void abortUpdate();
    
    // Web interface integration
    static void handleWebUpdate(WebServer& server);
    static void handleUpdateAPI(WebServer& server);
    static void handleRemoteUpdateAPI(WebServer& server);
    static String generateUpdatePage();
    
    // Advanced update methods
    static bool downloadFromURL(const String& url);
    static bool checkRemoteUpdates(const String& manifestUrl);
    static bool installFromBuffer(uint8_t* buffer, size_t size);
    static void setupUpdateRoutes(WebServer& server);
    
    // Status and monitoring
    static UpdateStatus getStatus();
    static const ProgressInfo& getProgress();
    static String getStatusString();
    static bool isUpdateAvailable();
    static const UpdateInfo& getAvailableUpdate();
    static String getProgressJSON();
    
    // Configuration
    static void enableAutoUpdate(bool enabled);
    static void setRollbackProtection(bool enabled);
    static void setMaxRetries(uint8_t maxRetries);
    
    // Error handling (public access)
    static String getLastError();
    static void clearErrors();
    
    // Callbacks
    using ProgressCallback = std::function<void(const ProgressInfo&)>;
    using StatusCallback = std::function<void(UpdateStatus, const String&)>;
    
    static void setProgressCallback(ProgressCallback callback);
    static void setStatusCallback(StatusCallback callback);
    
    // Utility functions
    static String getCurrentVersion();
    static String getBuildDate();
    static String getBuildTime();
    static uint32_t getSketchSize();
    static uint32_t getFreeSketchSpace();
    static bool hasEnoughSpace(size_t updateSize);

private:
    // Internal state
    static bool initialized;
    static String hostname;
    static String password;
    static String updateServerUrl;
    static unsigned long checkInterval;
    static unsigned long lastCheckTime;
    static bool autoUpdateEnabled;
    static bool rollbackProtectionEnabled;
    static uint8_t maxRetries;
    static uint8_t currentRetries;
    
    // Update state
    static UpdateStatus currentStatus;
    static UpdateInfo availableUpdate;
    static ProgressInfo progressInfo;
    static bool updateAvailable;
    
    // Callbacks
    static ProgressCallback progressCallback;
    static StatusCallback statusCallback;
    
    // Enhanced state management
    static HTTPClient httpClient;
    static uint8_t* downloadBuffer;
    static size_t downloadBufferSize;
    static String lastError;
    static String updateManifestUrl;
    static bool usePSRAM;
    
    // Internal methods
    static void setupArduinoOTA();
    static void setupWebOTA();
    static bool downloadUpdate(const String& url);
    static bool verifyChecksum(const String& expectedChecksum);
    static void updateProgress(size_t bytesReceived, size_t totalBytes);
    static void setStatus(UpdateStatus status, const String& message = "");
    static void onOTAStart();
    static void onOTAEnd();
    static void onOTAProgress(unsigned int progress, unsigned int total);
    static void onOTAError(ota_error_t error);
    static String getOTAErrorString(ota_error_t error);
    static void performRollback();
    static bool validateUpdate();
    
    // Version management
    static String parseVersionFromFilename(const String& filename);
    static bool isNewerVersion(const String& currentVersion, const String& newVersion);
    
    // Network helpers
    static bool downloadFile(const String& url, Stream& output);
    static bool downloadToBuffer(const String& url, uint8_t** buffer, size_t* size);
    static String httpGet(const String& url);
    static bool parseUpdateManifest(const String& json, UpdateInfo& updateInfo);
    static String calculateMD5(uint8_t* data, size_t length);
    static bool verifySignature(uint8_t* data, size_t length, const String& signature);
    
    // PSRAM utilization
    static uint8_t* allocatePSRAM(size_t size);
    static void deallocatePSRAM(uint8_t* ptr);
    static bool usePSRAMForDownloads();
    
    // Enhanced error handling (private)
    static void logError(const String& error);
    
    // Advanced download methods
    static bool downloadToBufferAndInstall(const String& url, size_t contentLength);
    static bool streamDownloadToFlash(size_t contentLength);
};

// Global instance for easy access
extern OTAManager OTA;