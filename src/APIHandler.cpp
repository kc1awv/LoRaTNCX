/**
 * @file APIHandler.cpp
 * @brief Implementation of base API handler class
 */

#include "APIHandler.h"

APIHandler::APIHandler(const String& basePath) : basePath(basePath)
{
}

void APIHandler::sendSuccess(AsyncWebServerRequest* request, const JsonDocument& data, const String& message)
{
    JsonDocument response = createSuccessResponse(data, message);
    sendJsonResponse(request, response);
}

void APIHandler::sendError(AsyncWebServerRequest* request, const String& message, int httpCode)
{
    JsonDocument response = createErrorResponse(message);
    sendJsonResponse(request, response, httpCode);
}

void APIHandler::sendJsonResponse(AsyncWebServerRequest* request, const JsonDocument& doc, int httpCode)
{
    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse* response = request->beginResponse(httpCode, "application/json", jsonString);
    setCORSHeaders(response);
    request->send(response);
}

void APIHandler::setCORSHeaders(AsyncWebServerResponse* response)
{
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    response->addHeader("Access-Control-Max-Age", "86400");
}

JsonDocument APIHandler::createErrorResponse(const String& message)
{
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = message;
    doc["timestamp"] = millis();
    return doc;
}

JsonDocument APIHandler::createSuccessResponse(const JsonDocument& data, const String& message)
{
    JsonDocument doc;
    doc["status"] = "success";
    
    if (!message.isEmpty()) {
        doc["message"] = message;
    }
    
    if (!data.isNull()) {
        doc["data"] = data;
    }
    
    doc["timestamp"] = millis();
    return doc;
}

bool APIHandler::validateRequiredParams(AsyncWebServerRequest* request, const std::vector<String>& requiredParams)
{
    for (const String& param : requiredParams) {
        if (!request->hasParam(param, true)) {
            return false;
        }
    }
    return true;
}

String APIHandler::getParam(AsyncWebServerRequest* request, const String& paramName, const String& defaultValue)
{
    if (request->hasParam(paramName, true)) {
        return request->getParam(paramName, true)->value();
    }
    return defaultValue;
}