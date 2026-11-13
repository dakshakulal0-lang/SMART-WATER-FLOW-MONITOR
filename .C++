#include <WiFi.h>
#include <WebServer.h>

// WiFi Credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Sensor Configuration
#define SENSOR_PIN 12

float calibrationFactor = 30.79;

volatile uint32_t pulseCount = 0;

float flowRate = 0.0;
float totalLiters = 0.0;

unsigned long lastTime = 0;

WebServer server(80);

// Interrupt Function
void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

// HTML Dashboard
void handleRoot() {

    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Smart Water Monitor</title>
    </head>
    <body>
        <h1>Smart Water Flow Monitor</h1>
        <h2>Flow Rate: <span id="flow">0</span> L/min</h2>
        <h2>Total Water: <span id="total">0</span> Liters</h2>

        <button onclick="fetch('/reset')">
            Reset Total
        </button>

        <script>
        setInterval(() => {
            fetch('/data')
            .then(res => res.json())
            .then(data => {
                document.getElementById('flow').innerText =
                    data.flowRate.toFixed(2);

                document.getElementById('total').innerText =
                    data.totalLiters.toFixed(3);
            });
        }, 1000);
        </script>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", html);
}

// Send Sensor Data
void handleData() {

    String json = "{\"flowRate\":" +
    String(flowRate, 2) +
    ",\"totalLiters\":" +
    String(totalLiters, 3) + "}";

    server.send(200, "application/json", json);
}

// Reset Water Consumption
void handleReset() {

    totalLiters = 0.0;

    server.send(200, "text/plain", "OK");
}

// Setup
void setup() {

    Serial.begin(115200);

    pinMode(SENSOR_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(SENSOR_PIN),
        pulseCounter,
        FALLING
    );

    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connected!");

    Serial.print("Dashboard IP: http://");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/reset", handleReset);

    server.begin();
}

// Main Loop
void loop() {

    server.handleClient();

    if (millis() - lastTime >= 1000) {

        detachInterrupt(
            digitalPinToInterrupt(SENSOR_PIN)
        );

        flowRate = pulseCount / calibrationFactor;

        if (pulseCount > 0) {
            totalLiters += flowRate / 60.0;
        }

        pulseCount = 0;

        lastTime = millis();

        attachInterrupt(
            digitalPinToInterrupt(SENSOR_PIN),
            pulseCounter,
            FALLING
        );
    }
}