#include "telemetry.h"

Telemetry::Telemetry() : server(80), ws("/ws"), jsonData(256), lastUpdateTime(0), lastKeyTime(0),
    keyUpdated(false), gamepadButtonUpdated(false), gamepadAxesUpdated(false),
    joystickLX(0.0), joystickLY(0.0), joystickRX(0.0), joystickRY(0.0), prevGamepadButton("") {}

void Telemetry::begin(const char* ssid, const char* password) {
    WiFi.softAP(ssid, password);
    Serial.print("ESP32 WiFi: ");
    Serial.println(ssid);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            clientCount++;
            Serial.println("Client connected, total: " + String(clientCount));
        } 
        else if (type == WS_EVT_DISCONNECT) {
            clientCount = max(clientCount - 1, 0);
            Serial.println("Client disconnected, total: " + String(clientCount));
        } 
        else if (type == WS_EVT_DATA) {
            String command = String((char*)data, len);
            if (command == "restart") {
                ESP.restart();
            } else if (command.startsWith("Keyboard:")) {
                latestKeyboardKey = command.substring(9);
                keyUpdated = true;
            } else if (command.startsWith("Gamepad: Buttons[")) {
                int buttonsStart = command.indexOf("[") + 1;
                int buttonsEnd = command.indexOf("]");
                String newGamepadButton = command.substring(buttonsStart, buttonsEnd);

                if (newGamepadButton != prevGamepadButton) {
                    latestGamepadButton = newGamepadButton;
                    prevGamepadButton = newGamepadButton;
                    gamepadButtonUpdated = true;
                } else if (newGamepadButton == "" && prevGamepadButton != "") {
                    latestGamepadButton = "";
                    prevGamepadButton = "";
                    gamepadButtonUpdated = true;
                }

                int axesStart = command.indexOf("Axes[") + 5;
                int axesEnd = command.length() - 1;
                String axesData = command.substring(axesStart, axesEnd);
                sscanf(axesData.c_str(), "Joystick_LX:%f, Joystick_LY:%f, Joystick_RX:%f, Joystick_RY:%f", 
                        &joystickLX, &joystickLY, &joystickRX, &joystickRY);
                joystickLY = -joystickLY;
                joystickRY = -joystickRY;
                gamepadAxesUpdated = true;
            }
        }
    });

    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", R"rawliteral(
            <!DOCTYPE html>
            <html lang="vi">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>ESP32 Telemetry & Control</title>
                <script>
                    let socket;
                    function connectWebSocket() {
                        socket = new WebSocket("ws://" + window.location.hostname + "/ws");

                        socket.onmessage = function (event) {
                            let data = JSON.parse(event.data);
                            let telemetryDiv = document.getElementById("telemetry");
                            telemetryDiv.innerHTML = "";

                            for (let key in data) {
                                telemetryDiv.innerHTML += `<p><strong>${key}:</strong> ${data[key]}</p>`;
                            }
                        };

                        socket.onclose = function () {
                            setTimeout(connectWebSocket, 1000);
                        };
                    }

                    function restartESP() {
                        socket.send("restart");
                    }

                    document.addEventListener("keydown", function(event) {
                        socket.send("Keyboard:" + event.key);
                    });

                    window.addEventListener("gamepadconnected", function(event) {
                        function sendGamepadData() {
                            let gp = navigator.getGamepads()[event.gamepad.index];
                            if (gp) {
                                let buttons = ["A", "B", "X", "Y", "L1", "R1", "L2", "R2", "Back", "Start", "LS", "RS", "Up", "Down", "Left", "Right"];
                                let buttonStates = gp.buttons.map((b, i) => b.pressed ? buttons[i] : "").filter(Boolean).join(",");
                                let axes = `Joystick_LX:${gp.axes[0].toFixed(2)}, Joystick_LY:${gp.axes[1].toFixed(2)}, Joystick_RX:${gp.axes[2].toFixed(2)}, Joystick_RY:${gp.axes[3].toFixed(2)}`;
                                
                                socket.send(`Gamepad: Buttons[${buttonStates}] Axes[${axes}]`);
                            }
                            requestAnimationFrame(sendGamepadData);
                        }
                        sendGamepadData();
                    });

                    window.onload = connectWebSocket;
                </script>
            </head>
            <body>
                <h2>ESP32 Telemetry & Control</h2>
                <div id="telemetry"></div>
                <button onclick="restartESP()">Restart ESP32</button>
            </body>
            </html>
        )rawliteral");
    });

    server.begin();
}
void Telemetry::addData(const char* key, float value) {
    jsonData[key] = value;
}

void Telemetry::addData(const char* key, const char* value) {
    jsonData[key] = value;
}

void Telemetry::addData(const char* key, String value) {
    jsonData[key] = value.c_str();
}

void Telemetry::update() {
    if (millis() - lastUpdateTime >= updateInterval && clientCount > 0) {
        String jsonStr;
        serializeJson(jsonData, jsonStr);
        ws.textAll(jsonStr);
        lastUpdateTime = millis();
    }
}

bool Telemetry::isClientConnected() {
    return clientCount > 0;
}
String Telemetry::getKeyboardKey() {
    if (keyUpdated) {
        keyUpdated = false;
        return latestKeyboardKey;
    }
    return "";
}

bool Telemetry::getGamepadButton(const char* button) {
    if (gamepadButtonUpdated && clientCount > 0) {
        return latestGamepadButton.indexOf(button) >= 0;
    }
    return false;
}

float Telemetry::getGamepadAxes(const char* axis) {

    if (strcmp(axis, "LeftX") == 0) return joystickLX;
    if (strcmp(axis, "LeftY") == 0) return joystickLY;
    if (strcmp(axis, "RightX") == 0) return joystickRX;
    if (strcmp(axis, "RightY") == 0) return joystickRY;
    return 0.0;
}

