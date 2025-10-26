#include "OTA.h"
#include <Preferences.h>

// Static member definitions
bool OTAManager::initialized = false;
String OTAManager::hostname = "loratnc";
String OTAManager::password = "";
String OTAManager::updateServerUrl = "";
unsigned long OTAManager::checkInterval = 24 * 60 * 60 * 1000; // 24 hours
unsigned long OTAManager::lastCheckTime = 0;
bool OTAManager::autoUpdateEnabled = false;
bool OTAManager::rollbackProtectionEnabled = true;
uint8_t OTAManager::maxRetries = 3;
uint8_t OTAManager::currentRetries = 0;

OTAManager::UpdateStatus OTAManager::currentStatus = UpdateStatus::IDLE;
OTAManager::UpdateInfo OTAManager::availableUpdate = {};
OTAManager::ProgressInfo OTAManager::progressInfo = {};
bool OTAManager::updateAvailable = false;

OTAManager::ProgressCallback OTAManager::progressCallback = nullptr;
OTAManager::StatusCallback OTAManager::statusCallback = nullptr;

// Enhanced state management
HTTPClient OTAManager::httpClient;
uint8_t* OTAManager::downloadBuffer = nullptr;
size_t OTAManager::downloadBufferSize = 0;
String OTAManager::lastError = "";
String OTAManager::updateManifestUrl = "";
bool OTAManager::usePSRAM = false;

void OTAManager::begin(const String& hostname_, uint16_t port)
{
    hostname = hostname_;
    
    if (!WiFi.isConnected()) {
        Serial.println(F("[OTA] WiFi not connected - OTA initialization skipped"));
        return;
    }
    
    setupArduinoOTA();
    ArduinoOTA.setPort(port);
    ArduinoOTA.begin();
    
    // Initialize progress info
    progressInfo.status = UpdateStatus::IDLE;
    progressInfo.bytesReceived = 0;
    progressInfo.totalBytes = 0;
    progressInfo.percentage = 0;
    progressInfo.statusMessage = "Ready";
    progressInfo.startTime = 0;
    progressInfo.estimatedTimeRemaining = 0;
    
    // Detect and configure PSRAM usage
    usePSRAM = psramFound() && ESP.getPsramSize() > 0;
    if (usePSRAM) {
        Serial.printf("[OTA] PSRAM detected: %d bytes available\n", ESP.getPsramSize());
    }
    
    // Load preferences
    Preferences prefs;
    prefs.begin("ota_config", true);
    autoUpdateEnabled = prefs.getBool("auto_update", false);
    rollbackProtectionEnabled = prefs.getBool("rollback_prot", true);
    maxRetries = prefs.getUChar("max_retries", 3);
    updateServerUrl = prefs.getString("server_url", "");
    updateManifestUrl = prefs.getString("manifest_url", "");
    prefs.end();
    
    initialized = true;
    setStatus(UpdateStatus::IDLE, "OTA Manager initialized");
    
    Serial.println(F("[OTA] OTA Manager initialized successfully"));
    Serial.printf("[OTA] Hostname: %s\n", hostname.c_str());
    Serial.printf("[OTA] Port: %d\n", port);
    Serial.printf("[OTA] Current version: %s\n", getCurrentVersion().c_str());
}

void OTAManager::setPassword(const String& password_)
{
    password = password_;
    if (initialized) {
        ArduinoOTA.setPassword(password.c_str());
    }
}

void OTAManager::setUpdateServer(const String& serverUrl)
{
    updateServerUrl = serverUrl;
    
    // Save to preferences
    Preferences prefs;
    prefs.begin("ota_config", false);
    prefs.putString("server_url", updateServerUrl);
    prefs.end();
}

void OTAManager::setCheckInterval(unsigned long intervalMs)
{
    checkInterval = intervalMs;
}

void OTAManager::handle()
{
    if (!initialized) return;
    
    ArduinoOTA.handle();
    
    // Check for automatic updates
    if (autoUpdateEnabled && !updateServerUrl.isEmpty()) {
        unsigned long now = millis();
        if (now - lastCheckTime > checkInterval) {
            checkForUpdates();
            lastCheckTime = now;
        }
    }
}

void OTAManager::checkForUpdates()
{
    // Simplified version - just check if server URL is configured
    if (updateServerUrl.isEmpty()) {
        Serial.println(F("[OTA] No update server configured"));
        return;
    }
    
    Serial.println(F("[OTA] Manual update check - use web interface at /update"));
    setStatus(UpdateStatus::IDLE, "Use web interface for updates");
}

bool OTAManager::startUpdate(const UpdateInfo& updateInfo)
{
    if (currentStatus != UpdateStatus::IDLE) {
        Serial.println(F("[OTA] Update already in progress"));
        return false;
    }
    
    if (!hasEnoughSpace(updateInfo.fileSize)) {
        setStatus(UpdateStatus::FAILED, "Insufficient space for update");
        return false;
    }
    
    currentRetries = 0;
    
    setStatus(UpdateStatus::DOWNLOADING, "Starting update download...");
    
    bool success = false;
    
    switch (updateInfo.source) {
        case UpdateSource::HTTP_DOWNLOAD:
            success = downloadUpdate(updateInfo.downloadUrl);
            break;
        case UpdateSource::WEB_UPLOAD:
            // Handled by web interface
            success = true;
            break;
        case UpdateSource::ARDUINO_OTA:
            // Handled by ArduinoOTA
            success = true;
            break;
    }
    
    if (success) {
        setStatus(UpdateStatus::SUCCESS, "Update completed successfully");
        Serial.println(F("[OTA] Update completed - restarting..."));
        delay(2000);
        ESP.restart();
    } else {
        currentRetries++;
        if (currentRetries < maxRetries) {
            Serial.printf("[OTA] Update failed, retry %d/%d\n", currentRetries, maxRetries);
            delay(5000);
            return startUpdate(updateInfo);
        } else {
            setStatus(UpdateStatus::FAILED, "Update failed after maximum retries");
            if (rollbackProtectionEnabled) {
                performRollback();
            }
        }
    }
    
    return success;
}

void OTAManager::abortUpdate()
{
    if (currentStatus == UpdateStatus::DOWNLOADING || 
        currentStatus == UpdateStatus::INSTALLING) {
        Update.abort();
        setStatus(UpdateStatus::IDLE, "Update aborted by user");
        Serial.println(F("[OTA] Update aborted"));
    }
}

void OTAManager::handleWebUpdate(WebServer& server)
{
    if (server.uri() == "/update") {
        if (server.method() == HTTP_GET) {
            server.send(200, "text/html", generateUpdatePage());
        } else if (server.method() == HTTP_POST) {
            server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            ESP.restart();
        } else {
            HTTPUpload& upload = server.upload();
            
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("[OTA] Web update started: %s\n", upload.filename.c_str());
                
                setStatus(UpdateStatus::INSTALLING, "Installing update via web...");
                progressInfo.startTime = millis();
                
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                    setStatus(UpdateStatus::FAILED, "Failed to begin update");
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                    setStatus(UpdateStatus::FAILED, "Write failed");
                } else {
                    updateProgress(upload.totalSize - upload.currentSize, upload.totalSize);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    Serial.printf("[OTA] Web update success: %u bytes\n", upload.totalSize);
                    setStatus(UpdateStatus::SUCCESS, "Web update completed");
                } else {
                    Update.printError(Serial);
                    setStatus(UpdateStatus::FAILED, "Update finalization failed");
                }
            }
        }
    }
}

String OTAManager::generateUpdatePage()
{
    String page = R"EOF(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🛰️ LoRa TNC - OTA Update</title>
    <style>
        :root {
            --primary: #2196f3;
            --success: #4caf50;
            --warning: #ff9800;
            --danger: #f44336;
            --dark: #333;
            --light: #f8f9fa;
        }
        
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
            overflow: hidden;
        }
        
        .header {
            background: var(--primary);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 { font-size: 2.5rem; margin-bottom: 10px; }
        .header p { opacity: 0.9; font-size: 1.1rem; }
        
        .content { padding: 40px; }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .info-card {
            background: var(--light);
            padding: 20px;
            border-radius: 8px;
            text-align: center;
            border-left: 4px solid var(--primary);
        }
        
        .info-card h3 { color: var(--dark); margin-bottom: 10px; }
        .info-card .value { font-size: 1.5rem; font-weight: bold; color: var(--primary); }
        
        .upload-section {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 30px;
            margin: 30px 0;
            border: 2px dashed #ddd;
            transition: all 0.3s ease;
        }
        
        .upload-section.dragover {
            border-color: var(--primary);
            background: #e3f2fd;
            transform: scale(1.02);
        }
        
        .upload-area {
            text-align: center;
            padding: 40px;
            cursor: pointer;
        }
        
        .upload-icon { font-size: 4rem; color: var(--primary); margin-bottom: 20px; }
        .upload-text { font-size: 1.2rem; color: var(--dark); margin-bottom: 10px; }
        .upload-hint { color: #666; }
        
        .file-input { display: none; }
        
        .progress-container {
            display: none;
            margin: 20px 0;
        }
        
        .progress-bar {
            background: #e0e0e0;
            border-radius: 10px;
            height: 20px;
            overflow: hidden;
            margin: 10px 0;
        }
        
        .progress-fill {
            background: linear-gradient(90deg, var(--success), var(--primary));
            height: 100%;
            width: 0%;
            transition: width 0.3s ease;
            position: relative;
        }
        
        .progress-text {
            text-align: center;
            margin: 10px 0;
            font-weight: bold;
        }
        
        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 6px;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s ease;
            text-decoration: none;
            display: inline-block;
        }
        
        .btn:hover { background: #1976d2; transform: translateY(-2px); }
        .btn:disabled { background: #ccc; cursor: not-allowed; }
        .btn-danger { background: var(--danger); }
        .btn-danger:hover { background: #d32f2f; }
        
        .status-message {
            padding: 15px;
            border-radius: 6px;
            margin: 15px 0;
            display: none;
        }
        
        .status-success { background: #e8f5e8; color: var(--success); border: 1px solid var(--success); }
        .status-error { background: #ffeaea; color: var(--danger); border: 1px solid var(--danger); }
        .status-info { background: #e3f2fd; color: var(--primary); border: 1px solid var(--primary); }
        
        .remote-update {
            background: #fff3e0;
            border: 1px solid #ff9800;
            border-radius: 8px;
            padding: 20px;
            margin: 20px 0;
        }
        
        .form-group {
            margin: 15px 0;
        }
        
        .form-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            color: var(--dark);
        }
        
        .form-control {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 1rem;
        }
        
        .form-control:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 5px rgba(33, 150, 243, 0.3);
        }
        
        @media (max-width: 600px) {
            .container { margin: 10px; }
            .content { padding: 20px; }
            .info-grid { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🛰️ OTA Update</h1>
            <p>LoRa KISS TNC Firmware Management</p>
        </div>
        
        <div class="content">
            <div class="info-grid">
                <div class="info-card">
                    <h3>Current Version</h3>
                    <div class="value">)EOF";
    
    page += getCurrentVersion();
    page += R"EOF(</div>
                </div>
                <div class="info-card">
                    <h3>Free Flash Space</h3>
                    <div class="value">)EOF";
    page += String(getFreeSketchSpace() / 1024);
    page += R"EOF( KB</div>
                </div>
                <div class="info-card">
                    <h3>Total Flash Size</h3>
                    <div class="value">)EOF";
    page += String(ESP.getFlashChipSize() / 1024);
    page += R"EOF( KB</div>
                </div>
                <div class="info-card">
                    <h3>PSRAM Available</h3>
                    <div class="value">)EOF";
    page += String(ESP.getPsramSize() / 1024);
    page += R"EOF( KB</div>
                </div>
            </div>

            <!-- File Upload Section -->
            <div class="upload-section" id="uploadSection">
                <div class="upload-area" onclick="document.getElementById('fileInput').click()">
                    <div class="upload-icon">📦</div>
                    <div class="upload-text">Drop firmware file here or click to browse</div>
                    <div class="upload-hint">Supports .bin files up to 8MB</div>
                </div>
                <input type="file" id="fileInput" class="file-input" accept=".bin" onchange="handleFileSelect(this.files[0])">
            </div>

            <!-- Progress Section -->
            <div class="progress-container" id="progressContainer">
                <div class="progress-text" id="progressText">Ready to upload...</div>
                <div class="progress-bar">
                    <div class="progress-fill" id="progressFill"></div>
                </div>
                <div style="text-align: center; margin-top: 15px;">
                    <button class="btn btn-danger" onclick="abortUpload()">Cancel Upload</button>
                </div>
            </div>

            <!-- Status Messages -->
            <div class="status-message" id="statusMessage"></div>

            <!-- Remote Update Section -->
            <div class="remote-update">
                <h3>🌐 Remote Update</h3>
                <p>Download firmware from a remote server</p>
                <div class="form-group">
                    <label for="updateUrl">Update Server URL:</label>
                    <input type="url" id="updateUrl" class="form-control" placeholder="https://example.com/firmware/update.json">
                </div>
                <button class="btn" onclick="checkRemoteUpdate()">Check for Updates</button>
                <button class="btn" onclick="downloadRemoteUpdate()" style="margin-left: 10px;">Download Latest</button>
            </div>

            <!-- Action Buttons -->
            <div style="text-align: center; margin-top: 30px;">
                <a href="/" class="btn">🏠 Back to Home</a>
                <button class="btn" onclick="location.reload()">🔄 Refresh Page</button>
            </div>
        </div>
    </div>

    <script>
        let uploadInProgress = false;
        let uploadAborted = false;

        // Drag and drop functionality
        const uploadSection = document.getElementById('uploadSection');
        const fileInput = document.getElementById('fileInput');
        const progressContainer = document.getElementById('progressContainer');
        const progressFill = document.getElementById('progressFill');
        const progressText = document.getElementById('progressText');
        const statusMessage = document.getElementById('statusMessage');

        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
            uploadSection.addEventListener(eventName, preventDefaults, false);
        });

        function preventDefaults(e) {
            e.preventDefault();
            e.stopPropagation();
        }

        ['dragenter', 'dragover'].forEach(eventName => {
            uploadSection.addEventListener(eventName, highlight, false);
        });

        ['dragleave', 'drop'].forEach(eventName => {
            uploadSection.addEventListener(eventName, unhighlight, false);
        });

        function highlight(e) {
            uploadSection.classList.add('dragover');
        }

        function unhighlight(e) {
            uploadSection.classList.remove('dragover');
        }

        uploadSection.addEventListener('drop', handleDrop, false);

        function handleDrop(e) {
            const dt = e.dataTransfer;
            const files = dt.files;
            if (files.length > 0) {
                handleFileSelect(files[0]);
            }
        }

        function handleFileSelect(file) {
            if (!file) return;

            if (!file.name.endsWith('.bin')) {
                showStatus('error', 'Please select a .bin firmware file');
                return;
            }

            if (file.size > 8 * 1024 * 1024) {
                showStatus('error', 'File too large. Maximum size is 8MB');
                return;
            }

            uploadFile(file);
        }

        function uploadFile(file) {
            if (uploadInProgress) {
                showStatus('error', 'Upload already in progress');
                return;
            }

            uploadInProgress = true;
            uploadAborted = false;
            progressContainer.style.display = 'block';
            uploadSection.style.display = 'none';

            const xhr = new XMLHttpRequest();
            const formData = new FormData();
            formData.append('update', file);

            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable && !uploadAborted) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    updateProgress(percentComplete, `Uploading: ${Math.round(percentComplete)}%`);
                }
            });

            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    updateProgress(100, 'Upload complete! Restarting device...');
                    showStatus('success', 'Firmware updated successfully. Device will restart in 5 seconds.');
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 5000);
                } else {
                    showStatus('error', 'Upload failed: ' + xhr.responseText);
                    resetUpload();
                }
            });

            xhr.addEventListener('error', function() {
                showStatus('error', 'Upload failed due to network error');
                resetUpload();
            });

            xhr.addEventListener('abort', function() {
                showStatus('info', 'Upload cancelled by user');
                resetUpload();
            });

            xhr.open('POST', '/update');
            xhr.send(formData);
        }

        function updateProgress(percent, text) {
            progressFill.style.width = percent + '%';
            progressText.textContent = text;
        }

        function abortUpload() {
            uploadAborted = true;
            // Note: Actual XHR abort would be implemented here
            resetUpload();
        }

        function resetUpload() {
            uploadInProgress = false;
            progressContainer.style.display = 'none';
            uploadSection.style.display = 'block';
            updateProgress(0, 'Ready to upload...');
        }

        function showStatus(type, message) {
            statusMessage.className = 'status-message status-' + type;
            statusMessage.textContent = message;
            statusMessage.style.display = 'block';
            
            if (type === 'success' || type === 'info') {
                setTimeout(() => {
                    statusMessage.style.display = 'none';
                }, 5000);
            }
        }

        function checkRemoteUpdate() {
            const url = document.getElementById('updateUrl').value;
            if (!url) {
                showStatus('error', 'Please enter a valid update server URL');
                return;
            }
            
            showStatus('info', 'Checking for remote updates...');
            // This would make an AJAX call to check for updates
            // Implementation would be added to handle JSON manifest checking
        }

        function downloadRemoteUpdate() {
            const url = document.getElementById('updateUrl').value;
            if (!url) {
                showStatus('error', 'Please enter a valid update server URL');
                return;
            }
            
            showStatus('info', 'Starting remote download...');
            // This would initiate a remote download
            // Implementation would be added to handle HTTP downloads
        }

        // Auto-refresh status every 5 seconds during upload
        setInterval(() => {
            if (uploadInProgress) {
                // Could fetch real-time status from device
            }
        }, 1000);
    </script>
</body>
</html>)EOF";
    
    return page;
}

OTAManager::UpdateStatus OTAManager::getStatus()
{
    return currentStatus;
}

const OTAManager::ProgressInfo& OTAManager::getProgress()
{
    return progressInfo;
}

String OTAManager::getStatusString()
{
    switch (currentStatus) {
        case UpdateStatus::IDLE: return "Ready";
        case UpdateStatus::CHECKING: return "Checking for updates...";
        case UpdateStatus::DOWNLOADING: return "Downloading update...";
        case UpdateStatus::INSTALLING: return "Installing update...";
        case UpdateStatus::SUCCESS: return "Update successful";
        case UpdateStatus::FAILED: return "Update failed";
        case UpdateStatus::ROLLBACK: return "Rolling back...";
        default: return "Unknown";
    }
}

bool OTAManager::isUpdateAvailable()
{
    return updateAvailable;
}

const OTAManager::UpdateInfo& OTAManager::getAvailableUpdate()
{
    return availableUpdate;
}

void OTAManager::enableAutoUpdate(bool enabled)
{
    autoUpdateEnabled = enabled;
    
    // Save to preferences
    Preferences prefs;
    prefs.begin("ota_config", false);
    prefs.putBool("auto_update", autoUpdateEnabled);
    prefs.end();
}

void OTAManager::setRollbackProtection(bool enabled)
{
    rollbackProtectionEnabled = enabled;
    
    // Save to preferences
    Preferences prefs;
    prefs.begin("ota_config", false);
    prefs.putBool("rollback_prot", rollbackProtectionEnabled);
    prefs.end();
}

void OTAManager::setMaxRetries(uint8_t maxRetries_)
{
    maxRetries = maxRetries_;
    
    // Save to preferences
    Preferences prefs;
    prefs.begin("ota_config", false);
    prefs.putUChar("max_retries", maxRetries);
    prefs.end();
}

void OTAManager::setProgressCallback(ProgressCallback callback)
{
    progressCallback = callback;
}

void OTAManager::setStatusCallback(StatusCallback callback)
{
    statusCallback = callback;
}

String OTAManager::getCurrentVersion()
{
    // Try to get version from build defines or use default
    #ifdef FIRMWARE_VERSION
        return String(FIRMWARE_VERSION);
    #else
        return "1.0.0+" + String(__DATE__) + "_" + String(__TIME__);
    #endif
}

String OTAManager::getBuildDate()
{
    return String(__DATE__);
}

String OTAManager::getBuildTime()
{
    return String(__TIME__);
}

uint32_t OTAManager::getSketchSize()
{
    return ESP.getSketchSize();
}

uint32_t OTAManager::getFreeSketchSpace()
{
    return ESP.getFreeSketchSpace();
}

bool OTAManager::hasEnoughSpace(size_t updateSize)
{
    return getFreeSketchSpace() > updateSize;
}

// Private methods implementation

void OTAManager::setupArduinoOTA()
{
    ArduinoOTA.setHostname(hostname.c_str());
    
    ArduinoOTA.onStart([]() {
        onOTAStart();
    });
    
    ArduinoOTA.onEnd([]() {
        onOTAEnd();
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        onOTAProgress(progress, total);
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        onOTAError(error);
    });
}

bool OTAManager::downloadUpdate(const String& url)
{
    Serial.printf("[OTA] Starting HTTP download from: %s\n", url.c_str());
    
    httpClient.begin(url);
    httpClient.setTimeout(30000); // 30 second timeout
    
    int httpCode = httpClient.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        lastError = "HTTP GET failed with code: " + String(httpCode);
        logError(lastError);
        httpClient.end();
        return false;
    }
    
    int contentLength = httpClient.getSize();
    if (contentLength <= 0) {
        lastError = "Invalid content length";
        logError(lastError);
        httpClient.end();
        return false;
    }
    
    if (!hasEnoughSpace(contentLength)) {
        lastError = "Insufficient flash space for update";
        logError(lastError);
        httpClient.end();
        return false;
    }
    
    // Use PSRAM for large downloads if available
    bool useBuffer = usePSRAM && contentLength <= (ESP.getPsramSize() / 2);
    
    if (useBuffer) {
        Serial.println(F("[OTA] Using PSRAM buffer for download"));
        return downloadToBufferAndInstall(url, contentLength);
    } else {
        Serial.println(F("[OTA] Streaming download directly to flash"));
        return streamDownloadToFlash(contentLength);
    }
}

bool OTAManager::downloadToBufferAndInstall(const String& url, size_t contentLength)
{
    // Allocate PSRAM buffer
    downloadBuffer = allocatePSRAM(contentLength);
    if (!downloadBuffer) {
        lastError = "Failed to allocate PSRAM buffer";
        logError(lastError);
        return false;
    }
    
    downloadBufferSize = contentLength;
    WiFiClient* stream = httpClient.getStreamPtr();
    
    setStatus(UpdateStatus::DOWNLOADING, "Downloading to PSRAM buffer...");
    progressInfo.totalBytes = contentLength;
    progressInfo.startTime = millis();
    
    size_t bytesRead = 0;
    while (httpClient.connected() && bytesRead < contentLength) {
        size_t available = stream->available();
        if (available > 0) {
            size_t toRead = min(available, contentLength - bytesRead);
            size_t actualRead = stream->readBytes(downloadBuffer + bytesRead, toRead);
            bytesRead += actualRead;
            
            updateProgress(bytesRead, contentLength);
            
            // Yield to prevent watchdog timeout
            if (bytesRead % 8192 == 0) {
                yield();
            }
        } else {
            delay(1);
        }
    }
    
    httpClient.end();
    
    if (bytesRead != contentLength) {
        lastError = "Download incomplete: " + String(bytesRead) + "/" + String(contentLength);
        logError(lastError);
        deallocatePSRAM(downloadBuffer);
        return false;
    }
    
    Serial.printf("[OTA] Download complete: %d bytes\n", bytesRead);
    
    // Install from buffer
    bool result = installFromBuffer(downloadBuffer, downloadBufferSize);
    
    // Clean up
    deallocatePSRAM(downloadBuffer);
    downloadBuffer = nullptr;
    downloadBufferSize = 0;
    
    return result;
}

bool OTAManager::streamDownloadToFlash(size_t contentLength)
{
    setStatus(UpdateStatus::INSTALLING, "Streaming download to flash...");
    
    if (!Update.begin(contentLength)) {
        lastError = "Failed to begin OTA update";
        logError(lastError);
        Update.printError(Serial);
        httpClient.end();
        return false;
    }
    
    WiFiClient* stream = httpClient.getStreamPtr();
    progressInfo.totalBytes = contentLength;
    progressInfo.startTime = millis();
    
    size_t written = 0;
    uint8_t buffer[1024];
    
    while (httpClient.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available > 0) {
            size_t toRead = min(available, sizeof(buffer));
            size_t actualRead = stream->readBytes(buffer, toRead);
            
            size_t actualWritten = Update.write(buffer, actualRead);
            if (actualWritten != actualRead) {
                lastError = "Flash write failed";
                logError(lastError);
                Update.abort();
                httpClient.end();
                return false;
            }
            
            written += actualWritten;
            updateProgress(written, contentLength);
            
            // Yield periodically
            if (written % 4096 == 0) {
                yield();
            }
        } else {
            delay(1);
        }
    }
    
    httpClient.end();
    
    if (written != contentLength) {
        lastError = "Download incomplete";
        logError(lastError);
        Update.abort();
        return false;
    }
    
    if (!Update.end(true)) {
        lastError = "Update finalization failed";
        logError(lastError);
        Update.printError(Serial);
        return false;
    }
    
    return true;
}

void OTAManager::updateProgress(size_t bytesReceived, size_t totalBytes)
{
    progressInfo.bytesReceived = bytesReceived;
    progressInfo.totalBytes = totalBytes;
    progressInfo.percentage = (totalBytes > 0) ? (bytesReceived * 100 / totalBytes) : 0;
    
    unsigned long elapsed = millis() - progressInfo.startTime;
    if (elapsed > 0 && bytesReceived > 0) {
        unsigned long rate = (bytesReceived * 1000) / elapsed; // bytes per second
        if (rate > 0) {
            progressInfo.estimatedTimeRemaining = ((totalBytes - bytesReceived) / rate) * 1000;
        }
    }
    
    if (progressCallback) {
        progressCallback(progressInfo);
    }
}

void OTAManager::setStatus(UpdateStatus status, const String& message)
{
    currentStatus = status;
    progressInfo.status = status;
    progressInfo.statusMessage = message;
    
    if (statusCallback) {
        statusCallback(status, message);
    }
}

void OTAManager::onOTAStart()
{
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    setStatus(UpdateStatus::INSTALLING, "Installing " + type + "...");
    Serial.println("[OTA] Start updating " + type);
    progressInfo.startTime = millis();
}

void OTAManager::onOTAEnd()
{
    setStatus(UpdateStatus::SUCCESS, "Arduino OTA update completed");
    Serial.println(F("[OTA] Arduino OTA End"));
}

void OTAManager::onOTAProgress(unsigned int progress, unsigned int total)
{
    updateProgress(progress, total);
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
}

void OTAManager::onOTAError(ota_error_t error)
{
    String errorMsg = "Arduino OTA Error: " + getOTAErrorString(error);
    setStatus(UpdateStatus::FAILED, errorMsg);
    Serial.println("[OTA] " + errorMsg);
}

String OTAManager::getOTAErrorString(ota_error_t error)
{
    switch (error) {
        case OTA_AUTH_ERROR: return "Auth Failed";
        case OTA_BEGIN_ERROR: return "Begin Failed";
        case OTA_CONNECT_ERROR: return "Connect Failed";
        case OTA_RECEIVE_ERROR: return "Receive Failed";
        case OTA_END_ERROR: return "End Failed";
        default: return "Unknown Error";
    }
}

void OTAManager::performRollback()
{
    setStatus(UpdateStatus::ROLLBACK, "Performing rollback...");
    Serial.println(F("[OTA] Performing rollback to previous firmware"));
    
    // ESP32 automatic rollback - just restart
    // The boot loader will automatically use the previous partition
    delay(2000);
    ESP.restart();
}

bool OTAManager::validateUpdate()
{
    // Basic validation - could be enhanced with checksum verification
    Serial.println(F("[OTA] Validating update..."));
    
    if (Update.hasError()) {
        Serial.println(F("[OTA] Update has errors"));
        Update.printError(Serial);
        return false;
    }
    
    return true;
}

bool OTAManager::isNewerVersion(const String& currentVersion, const String& newVersion)
{
    // Enhanced version comparison with semantic versioning support
    // Handle formats like "1.2.3", "v1.2.3", "1.2.3-beta", etc.
    
    String current = currentVersion;
    String newest = newVersion;
    
    // Remove 'v' prefix if present
    if (current.startsWith("v")) current = current.substring(1);
    if (newest.startsWith("v")) newest = newest.substring(1);
    
    // Split versions into parts
    int currentParts[3] = {0, 0, 0};
    int newestParts[3] = {0, 0, 0};
    
    // Parse current version
    int partIndex = 0;
    int startIndex = 0;
    for (int i = 0; i <= current.length() && partIndex < 3; i++) {
        if (i == current.length() || current.charAt(i) == '.' || current.charAt(i) == '-') {
            if (i > startIndex) {
                currentParts[partIndex] = current.substring(startIndex, i).toInt();
            }
            partIndex++;
            startIndex = i + 1;
        }
    }
    
    // Parse newest version
    partIndex = 0;
    startIndex = 0;
    for (int i = 0; i <= newest.length() && partIndex < 3; i++) {
        if (i == newest.length() || newest.charAt(i) == '.' || newest.charAt(i) == '-') {
            if (i > startIndex) {
                newestParts[partIndex] = newest.substring(startIndex, i).toInt();
            }
            partIndex++;
            startIndex = i + 1;
        }
    }
    
    // Compare version parts
    for (int i = 0; i < 3; i++) {
        if (newestParts[i] > currentParts[i]) return true;
        if (newestParts[i] < currentParts[i]) return false;
    }
    
    return false; // Versions are equal
}

// Enhanced Methods Implementation

bool OTAManager::downloadFromURL(const String& url)
{
    if (currentStatus != UpdateStatus::IDLE) {
        lastError = "Update already in progress";
        return false;
    }
    
    UpdateInfo updateInfo;
    updateInfo.downloadUrl = url;
    updateInfo.source = UpdateSource::HTTP_DOWNLOAD;
    updateInfo.fileSize = 0; // Will be determined during download
    
    return startUpdate(updateInfo);
}

bool OTAManager::checkRemoteUpdates(const String& manifestUrl)
{
    if (manifestUrl.isEmpty()) {
        lastError = "Empty manifest URL";
        return false;
    }
    
    setStatus(UpdateStatus::CHECKING, "Checking remote updates...");
    
    String manifestJson = httpGet(manifestUrl);
    if (manifestJson.isEmpty()) {
        lastError = "Failed to fetch update manifest";
        setStatus(UpdateStatus::IDLE, "Ready");
        return false;
    }
    
    UpdateInfo remoteUpdate;
    if (!parseUpdateManifest(manifestJson, remoteUpdate)) {
        lastError = "Failed to parse update manifest";
        setStatus(UpdateStatus::IDLE, "Ready");
        return false;
    }
    
    String currentVer = getCurrentVersion();
    if (isNewerVersion(currentVer, remoteUpdate.version)) {
        availableUpdate = remoteUpdate;
        updateAvailable = true;
        setStatus(UpdateStatus::IDLE, "Update available: " + remoteUpdate.version);
        Serial.printf("[OTA] Update available: %s -> %s\n", currentVer.c_str(), remoteUpdate.version.c_str());
        return true;
    } else {
        updateAvailable = false;
        setStatus(UpdateStatus::IDLE, "No updates available");
        Serial.println(F("[OTA] No updates available"));
        return false;
    }
}

bool OTAManager::installFromBuffer(uint8_t* buffer, size_t size)
{
    if (!buffer || size == 0) {
        lastError = "Invalid buffer or size";
        return false;
    }
    
    setStatus(UpdateStatus::INSTALLING, "Installing from buffer...");
    
    if (!Update.begin(size)) {
        lastError = "Failed to begin update from buffer";
        logError(lastError);
        Update.printError(Serial);
        return false;
    }
    
    // Write in chunks to show progress
    size_t written = 0;
    size_t chunkSize = 4096;
    
    progressInfo.totalBytes = size;
    progressInfo.startTime = millis();
    
    while (written < size) {
        size_t toWrite = min(chunkSize, size - written);
        
        size_t actualWritten = Update.write(buffer + written, toWrite);
        if (actualWritten != toWrite) {
            lastError = "Buffer write to flash failed";
            logError(lastError);
            Update.abort();
            return false;
        }
        
        written += actualWritten;
        updateProgress(written, size);
        
        // Yield periodically
        if (written % (chunkSize * 4) == 0) {
            yield();
        }
    }
    
    if (!Update.end(true)) {
        lastError = "Buffer update finalization failed";
        logError(lastError);
        Update.printError(Serial);
        return false;
    }
    
    Serial.printf("[OTA] Buffer install complete: %d bytes\n", written);
    return true;
}

void OTAManager::setupUpdateRoutes(WebServer& server)
{
    // Enhanced update API endpoint
    server.on("/api/update/status", HTTP_GET, [&server]() {
        handleUpdateAPI(server);
    });
    
    // Remote update endpoint
    server.on("/api/update/remote", HTTP_POST, [&server]() {
        handleRemoteUpdateAPI(server);
    });
}

void OTAManager::handleUpdateAPI(WebServer& server)
{
    String json = getProgressJSON();
    server.send(200, "application/json", json);
}

void OTAManager::handleRemoteUpdateAPI(WebServer& server)
{
    if (!server.hasArg("url")) {
        server.send(400, "application/json", "{\"error\":\"Missing URL parameter\"}");
        return;
    }
    
    String url = server.arg("url");
    bool success = false;
    
    if (url.endsWith(".json")) {
        // Check for updates from manifest
        success = checkRemoteUpdates(url);
    } else {
        // Direct download
        success = downloadFromURL(url);
    }
    
    String response = "{\"success\":" + String(success ? "true" : "false");
    if (!success && !lastError.isEmpty()) {
        response += ",\"error\":\"" + lastError + "\"";
    }
    response += "}";
    
    server.send(200, "application/json", response);
}

String OTAManager::getProgressJSON()
{
    String json = "{";
    json += "\"status\":\"" + getStatusString() + "\",";
    json += "\"statusCode\":" + String((int)currentStatus) + ",";
    json += "\"percentage\":" + String(progressInfo.percentage) + ",";
    json += "\"bytesReceived\":" + String(progressInfo.bytesReceived) + ",";
    json += "\"totalBytes\":" + String(progressInfo.totalBytes) + ",";
    json += "\"message\":\"" + progressInfo.statusMessage + "\",";
    json += "\"updateAvailable\":" + String(updateAvailable ? "true" : "false") + ",";
    json += "\"currentVersion\":\"" + getCurrentVersion() + "\",";
    json += "\"freeSpace\":" + String(getFreeSketchSpace()) + ",";
    json += "\"psramAvailable\":" + String(usePSRAM ? "true" : "false");
    if (updateAvailable) {
        json += ",\"availableVersion\":\"" + availableUpdate.version + "\"";
        json += ",\"updateDescription\":\"" + availableUpdate.description + "\"";
    }
    if (!lastError.isEmpty()) {
        json += ",\"lastError\":\"" + lastError + "\"";
    }
    json += "}";
    return json;
}

String OTAManager::httpGet(const String& url)
{
    httpClient.begin(url);
    httpClient.setTimeout(15000);
    
    int httpCode = httpClient.GET();
    String payload = "";
    
    if (httpCode == HTTP_CODE_OK) {
        payload = httpClient.getString();
    } else {
        lastError = "HTTP GET failed with code: " + String(httpCode);
        logError(lastError);
    }
    
    httpClient.end();
    return payload;
}

bool OTAManager::parseUpdateManifest(const String& json, UpdateInfo& updateInfo)
{
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        lastError = "JSON parsing failed: " + String(error.c_str());
        return false;
    }
    
    if (!doc.containsKey("version") || !doc.containsKey("url")) {
        lastError = "Invalid manifest: missing version or url";
        return false;
    }
    
    updateInfo.version = doc["version"].as<String>();
    updateInfo.downloadUrl = doc["url"].as<String>();
    updateInfo.description = doc.containsKey("description") ? doc["description"].as<String>() : "";
    updateInfo.checksum = doc.containsKey("checksum") ? doc["checksum"].as<String>() : "";
    updateInfo.fileSize = doc.containsKey("size") ? doc["size"].as<size_t>() : 0;
    updateInfo.forceUpdate = doc.containsKey("force") ? doc["force"].as<bool>() : false;
    updateInfo.source = UpdateSource::HTTP_DOWNLOAD;
    
    return true;
}

uint8_t* OTAManager::allocatePSRAM(size_t size)
{
    if (!usePSRAM || size == 0) {
        return nullptr;
    }
    
    uint8_t* ptr = (uint8_t*)ps_malloc(size);
    if (ptr) {
        Serial.printf("[OTA] Allocated %d bytes in PSRAM\n", size);
    } else {
        Serial.printf("[OTA] Failed to allocate %d bytes in PSRAM\n", size);
    }
    
    return ptr;
}

void OTAManager::deallocatePSRAM(uint8_t* ptr)
{
    if (ptr) {
        free(ptr);
        Serial.println(F("[OTA] Deallocated PSRAM"));
    }
}

bool OTAManager::usePSRAMForDownloads()
{
    return usePSRAM;
}

void OTAManager::logError(const String& error)
{
    lastError = error;
    Serial.println("[OTA ERROR] " + error);
}

String OTAManager::getLastError()
{
    return lastError;
}

void OTAManager::clearErrors()
{
    lastError = "";
}

// Global instance
OTAManager OTA;