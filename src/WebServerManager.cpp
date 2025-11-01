/**
 * @file WebServerManager.cpp
 * @brief Asynchronous web server management implementation
 */

#include "WebServerManager.h"
#include "SystemLogger.h"
#include <AsyncJson.h>
#include <ArduinoJson.h>

WebServerManager::WebServerManager(uint16_t port)
    : server(port), serverPort(port), serverRunning(false), filesystemMounted(false)
{
    // Initialize API manager
    apiManager = std::make_unique<APIManager>();
}

bool WebServerManager::begin()
{
    LOG_WEB_INFO("Initializing web server subsystem...");

    if (!initializeFilesystem())
    {
        LOG_WEB_ERROR("✗ Web server filesystem initialization failed");
        return false;
    }

    // Initialize API manager
    if (!apiManager->begin())
    {
        LOG_WEB_ERROR("✗ API manager initialization failed");
        return false;
    }

    setupRoutes();
    LOG_WEB_INFO("✓ Web server routes configured");
    LOG_WEB_INFO("✓ Web server subsystem initialized (server will start when WiFi is ready)");
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
        LOG_WEB_ERROR("✗ Cannot start web server - filesystem not mounted");
        return false;
    }

    LOG_WEB_INFO("Starting web server on port " + String(serverPort) + "...");
    
    server.begin();
    serverRunning = true;
    
    LOG_WEB_INFO("✓ Web server started successfully");
    LOG_WEB_INFO("✓ Web interface available at http://[device-ip]:" + String(serverPort));    return true;
}

void WebServerManager::stop()
{
    if (!serverRunning)
    {
        return;
    }

    LOG_WEB_INFO("Stopping web server...");
    server.end();
    serverRunning = false;
    LOG_WEB_INFO("✓ Web server stopped");
}

void WebServerManager::setCallbacks(
    std::function<String()> getSystemStatus,
    std::function<String()> getLoRaStatus,
    std::function<String()> getWiFiNetworks,
    std::function<bool(const String&, const String&, String&)> addWiFiNetwork,
    std::function<bool(const String&, String&)> removeWiFiNetwork)
{
    // Set callbacks through API manager
    if (apiManager) {
        apiManager->setSystemStatusCallback(getSystemStatus);
        apiManager->setLoRaStatusCallback(getLoRaStatus);
        apiManager->setWiFiNetworksCallback(getWiFiNetworks);
        apiManager->setAddWiFiNetworkCallback(addWiFiNetwork);
        apiManager->setRemoveWiFiNetworkCallback(removeWiFiNetwork);
    }
}

bool WebServerManager::initializeFilesystem()
{
    LOG_WEB_INFO("Mounting SPIFFS filesystem...");
    
    if (!SPIFFS.begin(true))  // Format if mount fails
    {
        LOG_WEB_ERROR("✗ SPIFFS mount failed");
        return false;
    }

    filesystemMounted = true;
    LOG_WEB_INFO("✓ SPIFFS mounted successfully");

    // List available files for debugging
    LOG_WEB_INFO("Available web files:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        LOG_WEB_INFO("  " + String(file.name()) + " (" + String(file.size()) + " bytes)");
        file = root.openNextFile();
    }

    return true;
}

void WebServerManager::setupRoutes()
{
    setupStaticFiles();
    
    // Register API routes through API manager
    if (apiManager) {
        apiManager->registerRoutes(server);
    }
    
    // Handle 404
    server.onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
    });
}

void WebServerManager::setupStaticFiles()
{
    // Serve root index.html
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (fileExists("/index.html"))
        {
            request->send(SPIFFS, "/index.html", "text/html");
        }
        else
        {
            request->send(200, "text/html", 
                "<h1>LoRaTNCX</h1><p>Web interface files not found. Upload web files to SPIFFS.</p>");
        }
    });

    // Serve static files with proper MIME types
    server.serveStatic("/css/", SPIFFS, "/css/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=86400"); // Cache CSS for 24 hours
    
    server.serveStatic("/js/", SPIFFS, "/js/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=86400"); // Cache JS for 24 hours
    
    server.serveStatic("/fonts/", SPIFFS, "/fonts/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=86400"); // Cache fonts for 24 hours

    // Serve any other static files from root directory
    server.serveStatic("/", SPIFFS, "/")
        .setDefaultFile("index.html");
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
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(response);
        return;
    }

    // For other requests, try to serve the file or return HTML 404
    String path = request->url();
    if (path.endsWith("/"))
    {
        path += "index.html";
    }

    // Try to serve file from root directory
    if (fileExists(path))
    {
        String mimeType = getMimeType(path);
        request->send(SPIFFS, path, mimeType);
    }
    else
    {
        request->send(404, "text/html", 
            "<h1>404 - Not Found</h1><p>The requested resource was not found.</p>");
    }
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