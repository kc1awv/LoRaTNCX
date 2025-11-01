/**
 * @file WebServerManager.cpp
 * @brief Asynchronous web server management implementation
 */

#include "WebServerManager.h"
#include <AsyncJson.h>
#include <ArduinoJson.h>

WebServerManager::WebServerManager(uint16_t port)
    : server(port), serverPort(port), serverRunning(false), filesystemMounted(false)
{
}

bool WebServerManager::begin()
{
    Serial.println("Initializing web server subsystem...");

    if (!initializeFilesystem())
    {
        Serial.println("✗ Web server filesystem initialization failed");
        return false;
    }

    setupRoutes();
    Serial.println("✓ Web server routes configured");
    Serial.println("✓ Web server subsystem initialized (server will start when WiFi is ready)");
    return true;
}

void WebServerManager::update()
{
    // The ESPAsyncWebServer handles everything asynchronously
    // We just need to manage start/stop based on WiFi state
    // This is handled by the main TNC manager calling start()/stop()
}

bool WebServerManager::start()
{
    if (serverRunning)
    {
        return true;
    }

    if (!filesystemMounted)
    {
        Serial.println("✗ Cannot start web server - filesystem not mounted");
        return false;
    }

    Serial.print("Starting web server on port ");
    Serial.print(serverPort);
    Serial.println("...");

    server.begin();
    serverRunning = true;

    Serial.println("✓ Web server started successfully");
    Serial.print("✓ Web interface available at http://[device-ip]:");
    Serial.println(serverPort);
    
    return true;
}

void WebServerManager::stop()
{
    if (!serverRunning)
    {
        return;
    }

    Serial.println("Stopping web server...");
    server.end();
    serverRunning = false;
    Serial.println("✓ Web server stopped");
}

void WebServerManager::setCallbacks(
    std::function<String()> getSystemStatus,
    std::function<String()> getLoRaStatus,
    std::function<String()> getWiFiNetworks,
    std::function<bool(const String&, const String&, String&)> addWiFiNetwork,
    std::function<bool(const String&, String&)> removeWiFiNetwork)
{
    getSystemStatusCallback = getSystemStatus;
    getLoRaStatusCallback = getLoRaStatus;
    getWiFiNetworksCallback = getWiFiNetworks;
    addWiFiNetworkCallback = addWiFiNetwork;
    removeWiFiNetworkCallback = removeWiFiNetwork;
}

bool WebServerManager::initializeFilesystem()
{
    Serial.println("Mounting SPIFFS filesystem...");
    
    if (!SPIFFS.begin(true))  // Format if mount fails
    {
        Serial.println("✗ SPIFFS mount failed");
        return false;
    }

    filesystemMounted = true;
    Serial.println("✓ SPIFFS mounted successfully");

    // List available files for debugging
    Serial.println("Available web files:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        Serial.print("  ");
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(" bytes)");
        file = root.openNextFile();
    }

    return true;
}

void WebServerManager::setupRoutes()
{
    setupStaticFiles();
    setupAPIRoutes();
    
    // Handle 404
    server.onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
    });
}

void WebServerManager::setupStaticFiles()
{
    // Serve root index.html
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (fileExists("/www/index.html"))
        {
            request->send(SPIFFS, "/www/index.html", "text/html");
        }
        else
        {
            request->send(200, "text/html", 
                "<h1>LoRaTNCX</h1><p>Web interface files not found. Upload web files to SPIFFS.</p>");
        }
    });

    // Serve static files with proper MIME types
    server.serveStatic("/css/", SPIFFS, "/www/css/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=86400"); // Cache CSS for 24 hours
    
    server.serveStatic("/js/", SPIFFS, "/www/js/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=86400"); // Cache JS for 24 hours

    // Serve any other static files from www directory
    server.serveStatic("/", SPIFFS, "/www/")
        .setDefaultFile("index.html");
}

void WebServerManager::setupAPIRoutes()
{
    // System status API
    server.on("/api/system/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleSystemStatus(request);
    });

    // LoRa status API
    server.on("/api/lora/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleLoRaStatus(request);
    });

    // WiFi networks API
    server.on("/api/wifi/networks", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleWiFiNetworks(request);
    });

    // Add WiFi network API
    server.on("/api/wifi/add", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleAddWiFiNetwork(request);
    });

    // Remove WiFi network API
    server.on("/api/wifi/remove", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleRemoveWiFiNetwork(request);
    });

    // Debug API - List SPIFFS files
    server.on("/api/debug/files", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleListFiles(request);
    });

    // Enable CORS for all API endpoints
    server.on("/api/*", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200);
        setCORSHeaders(response);
        request->send(response);
    });
}

void WebServerManager::handleSystemStatus(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    
    if (getSystemStatusCallback)
    {
        String status = getSystemStatusCallback();
        doc["status"] = "ok";
        doc["data"] = status;
        doc["uptime"] = millis();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["chip_model"] = ESP.getChipModel();
        doc["cpu_freq"] = ESP.getCpuFreqMHz();
    }
    else
    {
        doc["status"] = "error";
        doc["message"] = "System status callback not available";
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void WebServerManager::handleLoRaStatus(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    
    if (getLoRaStatusCallback)
    {
        String status = getLoRaStatusCallback();
        doc["status"] = "ok";
        doc["data"] = status;
    }
    else
    {
        doc["status"] = "error";
        doc["message"] = "LoRa status callback not available";
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void WebServerManager::handleWiFiNetworks(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    
    if (getWiFiNetworksCallback)
    {
        String networks = getWiFiNetworksCallback();
        doc["status"] = "ok";
        doc["data"] = networks;
    }
    else
    {
        doc["status"] = "error";
        doc["message"] = "WiFi networks callback not available";
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void WebServerManager::handleAddWiFiNetwork(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true))
    {
        doc["status"] = "error";
        doc["message"] = "Missing SSID or password parameter";
    }
    else if (!addWiFiNetworkCallback)
    {
        doc["status"] = "error";
        doc["message"] = "Add WiFi network callback not available";
    }
    else
    {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        String message;
        
        if (addWiFiNetworkCallback(ssid, password, message))
        {
            doc["status"] = "ok";
            doc["message"] = message;
        }
        else
        {
            doc["status"] = "error";
            doc["message"] = message;
        }
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void WebServerManager::handleRemoveWiFiNetwork(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    
    if (!request->hasParam("ssid", true))
    {
        doc["status"] = "error";
        doc["message"] = "Missing SSID parameter";
    }
    else if (!removeWiFiNetworkCallback)
    {
        doc["status"] = "error";
        doc["message"] = "Remove WiFi network callback not available";
    }
    else
    {
        String ssid = request->getParam("ssid", true)->value();
        String message;
        
        if (removeWiFiNetworkCallback(ssid, message))
        {
            doc["status"] = "ok";
            doc["message"] = message;
        }
        else
        {
            doc["status"] = "error";
            doc["message"] = message;
        }
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void WebServerManager::handleListFiles(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    doc["status"] = "ok";
    
    JsonArray files = doc["files"].to<JsonArray>();
    
    if (filesystemMounted)
    {
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while (file)
        {
            JsonObject fileObj = files.add<JsonObject>();
            fileObj["name"] = file.name();
            fileObj["size"] = file.size();
            file = root.openNextFile();
        }
    }
    else
    {
        doc["status"] = "error";
        doc["message"] = "Filesystem not mounted";
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void WebServerManager::handleNotFound(AsyncWebServerRequest *request)
{
    // For API requests, return JSON 404
    if (request->url().startsWith("/api/"))
    {
        JsonDocument doc;
        doc["status"] = "error";
        doc["message"] = "API endpoint not found";
        
        String jsonString;
        serializeJson(doc, jsonString);
        
        AsyncWebServerResponse *response = request->beginResponse(404, "application/json", jsonString);
        setCORSHeaders(response);
        request->send(response);
        return;
    }

    // For other requests, try to serve the file or return HTML 404
    String path = request->url();
    if (path.endsWith("/"))
    {
        path += "index.html";
    }

    // Try to serve file from /www/ directory
    String wwwPath = "/www" + path;
    if (fileExists(wwwPath))
    {
        String mimeType = getMimeType(path);
        request->send(SPIFFS, wwwPath, mimeType);
    }
    else
    {
        request->send(404, "text/html", 
            "<h1>404 - Not Found</h1><p>The requested resource was not found.</p>");
    }
}

void WebServerManager::setCORSHeaders(AsyncWebServerResponse *response)
{
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

String WebServerManager::getMimeType(const String& path)
{
    if (path.endsWith(".html") || path.endsWith(".htm"))
    {
        return "text/html";
    }
    else if (path.endsWith(".css"))
    {
        return "text/css";
    }
    else if (path.endsWith(".js"))
    {
        return "application/javascript";
    }
    else if (path.endsWith(".json"))
    {
        return "application/json";
    }
    else if (path.endsWith(".png"))
    {
        return "image/png";
    }
    else if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
    {
        return "image/jpeg";
    }
    else if (path.endsWith(".gif"))
    {
        return "image/gif";
    }
    else if (path.endsWith(".svg"))
    {
        return "image/svg+xml";
    }
    else if (path.endsWith(".ico"))
    {
        return "image/x-icon";
    }
    
    return "text/plain";
}

bool WebServerManager::fileExists(const String& path)
{
    return filesystemMounted && SPIFFS.exists(path);
}