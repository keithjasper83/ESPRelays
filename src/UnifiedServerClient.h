#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

using UnifiedJsonProvider = String (*)();
using UnifiedCommandHandler = bool (*)(bool desiredOn, String &error);
using UnifiedSettingsHandler = bool (*)(JsonObjectConst values, String &error);
using UnifiedCalibrationHandler = bool (*)(JsonObjectConst values, String &error);
using UnifiedRestartHandler = void (*)();

class UnifiedServerClient
{
public:
    void begin(UnifiedJsonProvider registrationProvider,
               UnifiedJsonProvider stateProvider,
               UnifiedCommandHandler commandHandler,
               UnifiedJsonProvider settingsProvider,
               UnifiedSettingsHandler settingsHandler,
               UnifiedJsonProvider calibrationProvider,
               UnifiedCalibrationHandler calibrationHandler,
               UnifiedRestartHandler restartHandler);
    bool configureFromServerUrl(const String &serverUrl, String &error);
    bool configureFromWebSocketUrl(const String &websocketUrl, String &error);
    void maintain(bool wifiConnected);
    void publishState();
    bool isConnected() const;
    String serverUrl() const;

private:
    void connect();
    void handleEvent(WStype_t type, uint8_t *payload, size_t length);
    void sendRegistration();
    void sendState(const char *messageType, const String &messageId = String());
    void sendSettings(const String &messageId, bool ok = true, const String &error = String());
    void sendCalibration(const String &messageId, bool ok = true, const String &error = String());
    void handleText(const uint8_t *payload, size_t length);
    bool parseWebSocketUrl(const String &url, String &host, uint16_t &port, String &path, String &error) const;

    WebSocketsClient socket;
    UnifiedJsonProvider registrationProvider = nullptr;
    UnifiedJsonProvider stateProvider = nullptr;
    UnifiedCommandHandler commandHandler = nullptr;
    UnifiedJsonProvider settingsProvider = nullptr;
    UnifiedSettingsHandler settingsHandler = nullptr;
    UnifiedJsonProvider calibrationProvider = nullptr;
    UnifiedCalibrationHandler calibrationHandler = nullptr;
    UnifiedRestartHandler restartHandler = nullptr;
    String configuredWebSocketUrl;
    String configuredServerUrl;
    bool connected = false;
    bool started = false;
    bool wifiWasConnected = false;
    unsigned long lastHeartbeatMs = 0;
};
