#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// Define pins for Motor A (DRV8833_1)
const int motorA_IN1 = 16; // DRV8833_1 AIN1
const int motorA_IN2 = 17; // DRV8833_1 AIN2

// Define pins for Motor B (DRV8833_1)
const int motorB_IN1 = 18; // DRV8833_1 BIN1
const int motorB_IN2 = 19; // DRV8833_1 BIN2

// Define pins for Motor C (DRV8833_2)
const int motorC_IN1 = 22; // DRV8833_2 AIN1
const int motorC_IN2 = 23; // DRV8833_2 AIN2

// Define pins for Motor D (DRV8833_2) - CORRECTED DEFINITIONS
const int motorD_IN3 = 25; // DRV8833_2 BIN1
const int motorD_IN4 = 26; // DRV8833_2 BIN2

// nSLEEP pins for DRV8833 modules
const int nSLEEP1_PIN = 2;  // DRV8833_1 nSLEEP
const int nSLEEP2_PIN = 27; // DRV8833_2 nSLEEP

// WiFi Credentials
const char* ssid = "ESP32_WebController";
const char* password = "password123";

// Server and WebSocket instances
WebServer server(80);
WebSocketsServer webSocket(81);

// --- Optimizations ---
int last_y_speed = 0;
const int SPEED_CHANGE_THRESHOLD = 5; 

void stopMotors(); // Forward declaration
void controlMotors(float y); // Forward declaration

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %d disconnected.\n", num);
      stopMotors();
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client %d connected from %s\n", num, ip.toString().c_str());
      webSocket.sendTXT(num, "Connected to ESP32!");
      break;
    }
      
    case WStype_TEXT: {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        float y = doc["y"]; 
        controlMotors(y);
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
      break;
    }
    default:
        break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Optimized 4-Motor Joystick Control");

  // Set motor control pins as outputs
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorB_IN1, OUTPUT);
  pinMode(motorB_IN2, OUTPUT);
  pinMode(motorC_IN1, OUTPUT);
  pinMode(motorC_IN2, OUTPUT);
  pinMode(motorD_IN3, OUTPUT); // CORRECTED
  pinMode(motorD_IN4, OUTPUT); // CORRECTED
  pinMode(nSLEEP1_PIN, OUTPUT);
  pinMode(nSLEEP2_PIN, OUTPUT);

  // Enable DRV8833 modules
  digitalWrite(nSLEEP1_PIN, HIGH);
  digitalWrite(nSLEEP2_PIN, HIGH);

  // Initial state: motors stopped
  stopMotors();

  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start Web Server
  server.begin();
  Serial.println("HTTP server started.");

  // Start WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started.");
  
  Serial.println("Ready! Connect to the WiFi and open the IP address in a browser.");
}

void loop() {
  webSocket.loop();
  server.handleClient();
}

/**
 * @brief Controls all four motors based on the y-axis joystick value.
 */
void controlMotors(float y) {
  int current_speed = (int)(y * 255);

  if (abs(current_speed - last_y_speed) < SPEED_CHANGE_THRESHOLD) {
    return; // No significant change, do nothing.
  }

  last_y_speed = current_speed;

  if (current_speed > 0) { // Move Forward
    Serial.printf("[Motors] Forward: %d%%\n", (int)(y * 100));
    analogWrite(motorA_IN1, current_speed);
    analogWrite(motorA_IN2, 0);
    analogWrite(motorB_IN1, current_speed);
    analogWrite(motorB_IN2, 0);
    analogWrite(motorC_IN1, current_speed);
    analogWrite(motorC_IN2, 0);
    analogWrite(motorD_IN3, current_speed); // CORRECTED
    analogWrite(motorD_IN4, 0);             // CORRECTED
  } else if (current_speed < 0) { // Move Backward
    int backward_speed = -current_speed;
    Serial.printf("[Motors] Backward: %d%%\n", (int)(-y * 100));
    analogWrite(motorA_IN1, 0);
    analogWrite(motorA_IN2, backward_speed);
    analogWrite(motorB_IN1, 0);
    analogWrite(motorB_IN2, backward_speed);
    analogWrite(motorC_IN1, 0);
    analogWrite(motorC_IN2, backward_speed);
    analogWrite(motorD_IN3, 0);              // CORRECTED
    analogWrite(motorD_IN4, backward_speed); // CORRECTED
  } else { // Stop
    stopMotors();
  }
}

/**
 * @brief Stops all motors immediately and prints status.
 */
void stopMotors() {
  if (last_y_speed != 0) {
      Serial.println("[Motors] Stopped");
  }
  last_y_speed = 0;
  analogWrite(motorA_IN1, 0);
  analogWrite(motorA_IN2, 0);
  analogWrite(motorB_IN1, 0);
  analogWrite(motorB_IN2, 0);
  analogWrite(motorC_IN1, 0);
  analogWrite(motorC_IN2, 0);
  analogWrite(motorD_IN3, 0); // CORRECTED
  analogWrite(motorD_IN4, 0); // CORRECTED
}