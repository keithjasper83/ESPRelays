#include "UnifiedServerClient.h"

#include <ArduinoJson.h>
#include <WiFi.h>

namespace
{
    constexpr unsigned long HEARTBEAT_INTERVAL_MS = 30000UL;
}

void UnifiedServerClient::begin(UnifiedJsonProvider newRegistrationProvider,
                                UnifiedJsonProvider newStateProvider,
                                UnifiedCommandHandler newCommandHandler,
                                UnifiedJsonProvider newSettingsProvider,
                                UnifiedSettingsHandler newSettingsHandler,
                                UnifiedJsonProvider newCalibrationProvider,
                                UnifiedCalibrationHandler newCalibrationHandler,
                                UnifiedRestartHandler newRestartHandler)
{
    registrationProvider = newRegistrationProvider;
    stateProvider = newStateProvider;
    commandHandler = newCommandHandler;
    settingsProvider = newSettingsProvider;
    settingsHandler = newSettingsHandler;
    calibrationProvider = newCalibrationProvider;
    calibrationHandler = newCalibrationHandler;
    restartHandler = newRestartHandler;
    socket.onEvent([this](WStype_t type, uint8_t *payload, size_t length)
                   { handleEvent(type, payload, length); });
    socket.setReconnectInterval(5000);
    socket.enableHeartbeat(15000, 4000, 2);
}

bool UnifiedServerClient::parseWebSocketUrl(const String &url, String &host, uint16_t &port, String &path, String &error) const
{
    if (!url.startsWith("ws://"))
    {
        error = "Only local ws:// Unified Server URLs are supported";
        return false;
    }
    const int authorityStart = 5;
    int pathStart = url.indexOf('/', authorityStart);
    String authority = pathStart >= 0 ? url.substring(authorityStart, pathStart) : url.substring(authorityStart);
    path = pathStart >= 0 ? url.substring(pathStart) : String("/ws/device");
    const int colon = authority.lastIndexOf(':');
    if (colon > 0)
    {
        host = authority.substring(0, colon);
        const long parsed = authority.substring(colon + 1).toInt();
        if (parsed <= 0 || parsed > 65535)
        {
            error = "Invalid Unified Server port";
            return false;
        }
        port = static_cast<uint16_t>(parsed);
    }
    else
    {
        host = authority;
        port = 80;
    }
    if (host.length() == 0)
    {
        error = "Unified Server host is empty";
        return false;
    }
    return true;
}

bool UnifiedServerClient::configureFromServerUrl(const String &serverUrl, String &error)
{
    String normalized = serverUrl;
    normalized.trim();
    if (!normalized.startsWith("http://"))
    {
        error = "Only local http:// Unified Server URLs are supported";
        return false;
    }
    while (normalized.endsWith("/"))
    {
        normalized.remove(normalized.length() - 1);
    }
    configuredServerUrl = normalized;
    String websocketUrl = "ws://" + normalized.substring(7) + "/ws/device";
    return configureFromWebSocketUrl(websocketUrl, error);
}

bool UnifiedServerClient::configureFromWebSocketUrl(const String &websocketUrl, String &error)
{
    String host;
    String path;
    uint16_t port = 0;
    if (!parseWebSocketUrl(websocketUrl, host, port, path, error))
    {
        return false;
    }
    if (configuredWebSocketUrl == websocketUrl && started)
    {
        return true;
    }
    socket.disconnect();
    connected = false;
    started = false;
    configuredWebSocketUrl = websocketUrl;
    if (configuredServerUrl.length() == 0)
    {
        configuredServerUrl = "http://" + host + (port == 80 ? String() : String(":") + String(port));
    }
    socket.begin(host, port, path);
    started = true;
    Serial.printf("[UNIFIED] Configured server %s\n", configuredServerUrl.c_str());
    return true;
}

void UnifiedServerClient::maintain(bool wifiConnected)
{
    if (!wifiConnected)
    {
        connected = false;
        wifiWasConnected = false;
        return;
    }
    if (!started)
    {
        return;
    }
    if (!wifiWasConnected)
    {
        wifiWasConnected = true;
    }
    socket.loop();
    const unsigned long now = millis();
    if (connected && (now - lastHeartbeatMs) >= HEARTBEAT_INTERVAL_MS)
    {
        sendState("heartbeat");
        lastHeartbeatMs = now;
    }
}

void UnifiedServerClient::publishState()
{
    if (connected)
    {
        sendState("state");
    }
}

bool UnifiedServerClient::isConnected() const
{
    return connected;
}

String UnifiedServerClient::serverUrl() const
{
    return configuredServerUrl;
}

void UnifiedServerClient::handleEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_CONNECTED)
    {
        connected = true;
        lastHeartbeatMs = millis();
        Serial.println("[UNIFIED] WebSocket connected");
        sendRegistration();
    }
    else if (type == WStype_DISCONNECTED)
    {
        connected = false;
        Serial.println("[UNIFIED] WebSocket disconnected");
    }
    else if (type == WStype_TEXT)
    {
        handleText(payload, length);
    }
}

void UnifiedServerClient::sendRegistration()
{
    if (registrationProvider == nullptr)
    {
        return;
    }
    const String data = registrationProvider();
    String message = "{\"protocol\":\"kj-esp-unified\",\"protocol_version\":1,\"type\":\"register\",\"id\":\"boot-";
    message += String(millis());
    message += "\",\"data\":" + data + "}";
    socket.sendTXT(message);
}

void UnifiedServerClient::sendState(const char *messageType, const String &messageId)
{
    if (stateProvider == nullptr)
    {
        return;
    }
    String message = "{\"protocol\":\"kj-esp-unified\",\"protocol_version\":1,\"type\":\"";
    message += messageType;
    message += "\",\"id\":\"";
    message += messageId.length() > 0 ? messageId : String(millis());
    message += "\",\"data\":" + stateProvider() + "}";
    socket.sendTXT(message);
}

void UnifiedServerClient::sendSettings(const String &messageId, bool ok, const String &error)
{
    JsonDocument response;
    if (settingsProvider != nullptr)
    {
        deserializeJson(response, settingsProvider());
    }
    response["ok"] = ok;
    if (!ok)
    {
        response["error"] = error;
    }
    String data;
    serializeJson(response, data);
    String message = "{\"protocol\":\"kj-esp-unified\",\"protocol_version\":1,\"type\":\"settings\",\"id\":\"";
    message += messageId;
    message += "\",\"data\":" + data + "}";
    socket.sendTXT(message);
}

void UnifiedServerClient::sendCalibration(const String &messageId, const bool ok, const String &error)
{
    String message = "{\"protocol\":\"kj-esp-unified\",\"protocol_version\":1,\"type\":\"calibration\",\"id\":\"";
    message += messageId;
    message += "\",\"data\":{\"ok\":";
    message += ok ? "true" : "false";
    if (calibrationProvider != nullptr)
    {
        message += ",\"calibration\":" + calibrationProvider();
    }
    if (!ok)
    {
        String escaped = error;
        escaped.replace("\\", "\\\\");
        escaped.replace("\"", "\\\"");
        message += ",\"error\":\"" + escaped + "\"";
    }
    message += "}}";
    socket.sendTXT(message);
}

void UnifiedServerClient::handleText(const uint8_t *payload, size_t length)
{
    JsonDocument document;
    const DeserializationError parseError = deserializeJson(document, payload, length);
    if (parseError)
    {
        return;
    }
    const String type = document["type"] | "";
    const String messageId = document["id"] | String(millis());
    if (type == "status_request")
    {
        sendState("state", messageId);
        return;
    }
    if (type == "settings_get")
    {
        sendSettings(messageId);
        return;
    }
    if (type == "settings_set")
    {
        String error;
        const JsonObjectConst values = document["data"].as<JsonObjectConst>();
        const bool ok = settingsHandler != nullptr && settingsHandler(values, error);
        sendSettings(messageId, ok, error);
        if (ok)
        {
            publishState();
        }
        return;
    }
    if (type == "calibration_get")
    {
        sendCalibration(messageId);
        return;
    }
    if (type == "calibration_set")
    {
        String error;
        const JsonObjectConst values = document["data"].as<JsonObjectConst>();
        const bool ok = calibrationHandler != nullptr && calibrationHandler(values, error);
        sendCalibration(messageId, ok, error);
        if (ok) publishState();
        return;
    }
    if (type == "restart")
    {
        const bool ok = restartHandler != nullptr;
        String response = "{\"protocol\":\"kj-esp-unified\",\"protocol_version\":1,\"type\":\"restart_result\",\"id\":\"";
        response += messageId;
        response += "\",\"data\":{\"ok\":";
        response += ok ? "true" : "false";
        response += ok ? ",\"scheduled_in_ms\":1000}}" : ",\"error\":\"restart unavailable\"}}";
        socket.sendTXT(response);
        if (ok)
        {
            restartHandler();
        }
        return;
    }
    if (type != "command")
    {
        return;
    }

    const String desiredState = document["data"]["state"] | "";
    bool desiredOn = desiredState == "on";
    String error;
    const bool valid = desiredState == "on" || desiredState == "off";
    const bool ok = valid && commandHandler != nullptr && commandHandler(desiredOn, error);
    String response = "{\"protocol\":\"kj-esp-unified\",\"protocol_version\":1,\"type\":\"command_result\",\"id\":\"";
    response += messageId;
    response += "\",\"data\":{\"ok\":";
    response += ok ? "true" : "false";
    response += ",\"state\":\"" + desiredState + "\"";
    if (!ok)
    {
        response += ",\"error\":\"" + (valid ? error : String("state must be on or off")) + "\"";
    }
    response += "}}";
    socket.sendTXT(response);
}
