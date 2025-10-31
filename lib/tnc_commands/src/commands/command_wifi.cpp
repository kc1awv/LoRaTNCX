#include "CommandContext.h"

TNCCommandResult TNCCommands::handleWIFI(const String args[], int argCount)
{
    if (!wifiAddCallback || !wifiRemoveCallback)
    {
        sendResponse("WiFi control is not available on this build");
        return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
    }

    if (argCount == 0)
    {
        String status;
        if (wifiStatusCallback)
        {
            wifiStatusCallback(status);
        }
        else
        {
            status = "WiFi status unavailable";
        }
        sendResponse(status);
        return TNCCommandResult::SUCCESS;
    }

    String action = toUpperCase(args[0]);

    if (action == "STATUS")
    {
        String status;
        if (wifiStatusCallback)
        {
            wifiStatusCallback(status);
        }
        else
        {
            status = "WiFi status unavailable";
        }
        sendResponse(status);
        return TNCCommandResult::SUCCESS;
    }
    else if (action == "LIST")
    {
        if (wifiListCallback)
        {
            String list;
            wifiListCallback(list);
            sendResponse(list);
            return TNCCommandResult::SUCCESS;
        }
        sendResponse("WiFi list unavailable");
        return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
    }
    else if (action == "ADD")
    {
        if (argCount < 3)
        {
            sendResponse("Usage: WIFI ADD <SSID> <PASSWORD>");
            return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
        }

        String message;
        if (wifiAddCallback(args[1], args[2], message))
        {
            sendResponse(message);
            return TNCCommandResult::SUCCESS;
        }

        sendResponse("ERROR: " + message);
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    else if (action == "REMOVE" || action == "DELETE" || action == "DEL")
    {
        if (argCount < 2)
        {
            sendResponse("Usage: WIFI REMOVE <SSID>");
            return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
        }

        String message;
        if (wifiRemoveCallback(args[1], message))
        {
            sendResponse(message);
            return TNCCommandResult::SUCCESS;
        }

        sendResponse("ERROR: " + message);
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }

    sendResponse("Usage: WIFI [STATUS|LIST|ADD <SSID> <PASSWORD>|REMOVE <SSID>]");
    return TNCCommandResult::ERROR_INVALID_PARAMETER;
}

