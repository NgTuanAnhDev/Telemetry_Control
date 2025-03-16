#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class Telemetry {
  private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    DynamicJsonDocument jsonData;  
    unsigned long lastUpdateTime;
    const unsigned long updateInterval = 100;
    String latestKeyboardKey;
    String latestGamepadButton;
    String prevGamepadButton;
    float joystickLX, joystickLY, joystickRX, joystickRY;
    bool keyUpdated;
    bool gamepadButtonUpdated;
    bool gamepadAxesUpdated;
    unsigned long lastKeyTime;
    int clientCount = 0;

  public:
    Telemetry();
    void begin(const char* ssid, const char* password);
    void addData(const char* key, float value);
    void update();
    String getKeyboardKey();
    bool getGamepadButton(const char* button);
    float getGamepadAxes(const char* axis);
    bool isClientConnected();
};

#endif
