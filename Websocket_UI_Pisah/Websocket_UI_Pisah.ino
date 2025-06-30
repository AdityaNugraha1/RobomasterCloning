#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WebController";
const char* password = "password123";

WebSocketsServer webSocket(81);

String serialBuffer;
bool wsConnected = false;
int connectedClients = 0;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  unsigned long startTime = micros();
  
  switch(type) {
    case WStype_DISCONNECTED:
      connectedClients--;
      Serial.printf("[WS] Client %d disconnected from IP: %s | Clients connected: %d | Latency: %.3f ms\n", 
        num, webSocket.remoteIP(num).toString().c_str(), connectedClients, (float)(micros() - startTime) / 1000.0);
      wsConnected = (connectedClients > 0);
      break;
      
    case WStype_CONNECTED:
      connectedClients++;
      Serial.printf("[WS] Client %d connected from IP: %s | Clients connected: %d | Latency: %.3f ms\n", 
        num, webSocket.remoteIP(num).toString().c_str(), connectedClients, (float)(micros() - startTime) / 1000.0);
      wsConnected = true;
      webSocket.sendTXT(num, "[ESP] Connected! Send messages via Web");
      break;
      
    case WStype_TEXT:
      Serial.printf("[Web] Message from client %d (IP: %s): %s | Latency: %.3f ms\n", 
        num, webSocket.remoteIP(num).toString().c_str(), (char*)payload, (float)(micros() - startTime) / 1000.0);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // Start WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  
  // Read from Serial and send to Web with logging
  unsigned long startTime = micros();
  while(Serial.available()) {
    char c = Serial.read();
    if(c == '\n') {
      if(serialBuffer.length() > 0 && wsConnected) {
        float latency = (float)(micros() - startTime) / 1000.0;
        String message = "[Serial] " + serialBuffer;
        Serial.printf("[Serial] Sending: %s | Latency: %.3f ms\n", message.c_str(), latency);
        webSocket.broadcastTXT(message);
        serialBuffer = "";
      }
    } else if(c != '\r') {
      serialBuffer += c;
    }
  }
}