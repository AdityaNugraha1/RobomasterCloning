#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// Define pins for Motor A
const int motorA_IN1 = 22; // Connect to DRV8833 IN1
const int motorA_IN2 = 23; // Connect to DRV8833 IN2

// Define pins for Motor B
const int motorB_IN3 = 25; // Connect to DRV8833 IN3
const int motorB_IN4 = 26; // Connect to DRV8833 IN4

// nSLEEP pin for DRV8833
const int nSLEEP_PIN = 27; // Connect to DRV8833 nSLEEP

const char* ssid = "ESP32_WebController";
const char* password = "password123";

WebServer server(80);
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
        num, webSocket.remoteIP(num).toString().c_str(), connectedClients, (float)(micros() - startTime) / 1000.0, Serial.print("Initial Free Heap: "), Serial.print(ESP.getFreeHeap()), Serial.println(" bytes"));
      wsConnected = (connectedClients > 0);
      break;
      
    case WStype_CONNECTED:
      connectedClients++;
      Serial.printf("[WS] Client %d connected from IP: %s | Clients connected: %d | Latency: %.3f ms\n", 
        num, webSocket.remoteIP(num).toString().c_str(), connectedClients, (float)(micros() - startTime) / 1000.0, Serial.print("Initial Free Heap: "), Serial.print(ESP.getFreeHeap()), Serial.println(" bytes"));
      wsConnected = true;
      webSocket.sendTXT(num, "[ESP] Connected! Use buttons to control motors");
      break;
      
    case WStype_TEXT:
      String message = (char*)payload;
      Serial.printf("[Web] Message from client %d (IP: %s): %s | Latency: %.3f ms\n", 
        num, webSocket.remoteIP(num).toString().c_str(), message.c_str(), (float)(micros() - startTime) / 1000.0);
        
      // Process motor control commands
      if (message == "motorA_forward") {
        moveMotorA_Forward();
        webSocket.broadcastTXT("[Motor A] Moving Forward");
      } else if (message == "motorA_backward") {
        moveMotorA_Backward();
        webSocket.broadcastTXT("[Motor A] Moving Backward");
      } else if (message == "motorA_stop") {
        stopMotorA();
        webSocket.broadcastTXT("[Motor A] Stopped");
      } else if (message == "motorB_forward") {
        moveMotorB_Forward();
        webSocket.broadcastTXT("[Motor B] Moving Forward");
      } else if (message == "motorB_backward") {
        moveMotorB_Backward();
        webSocket.broadcastTXT("[Motor B] Moving Backward");
      } else if (message == "motorB_stop") {
        stopMotorB();
        webSocket.broadcastTXT("[Motor B] Stopped");
      }
      break;
  }
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Motor Control Remote</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(to bottom, #e0e7ff, #f3f4f6);
      margin: 0;
      padding: 20px;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
    }
    .container {
      width: 100%;
      max-width: 600px;
      background: #fff;
      border-radius: 10px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.1);
      padding: 20px;
    }
    h1 {
      font-size: 1.5rem;
      color: #1f2937;
      margin-bottom: 1rem;
      text-align: center;
    }
    .status {
      font-weight: bold;
      margin-bottom: 1rem;
      text-align: center;
    }
    .connected { color: #16a34a; }
    .disconnected { color: #dc2626; }
    #messages {
      height: 200px;
      overflow-y: auto;
      background: #f9fafb;
      border: 1px solid #e5e7eb;
      border-radius: 8px;
      padding: 10px;
      margin-bottom: 1rem;
      scroll-behavior: smooth;
    }
    .message {
      margin-bottom: 0.5rem;
      padding: 8px;
      border-radius: 5px;
      animation: fadeIn 0.3s ease-in;
    }
    .system { color: #6b7280; }
    .motor { background: #ecfdf5; color: #16a34a; }
    .control-group {
      display: flex;
      flex-direction: column;
      gap: 10px;
      margin-bottom: 1rem;
    }
    .motor-controls {
      display: flex;
      justify-content: space-between;
      gap: 20px;
    }
    .motor-box {
      flex: 1;
      padding: 10px;
      border: 1px solid #d1d5db;
      border-radius: 8px;
      text-align: center;
    }
    .motor-box h2 {
      font-size: 1.2rem;
      margin-bottom: 10px;
    }
    button {
      padding: 10px;
      border: none;
      border-radius: 5px;
      font-size: 1rem;
      cursor: pointer;
      transition: background 0.2s;
      width: 100%;
      margin: 5px 0;
    }
    .forward-btn { background: #2563eb; color: #fff; }
    .forward-btn:hover { background: #1d4ed8; }
    .backward-btn { background: #f59e0b; color: #fff; }
    .backward-btn:hover { background: #d97706; }
    .stop-btn { background: #dc2626; color: #fff; }
    .stop-btn:hover { background: #b91c1c; }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    @media (max-width: 600px) {
      .container { padding: 15px; }
      h1 { font-size: 1.25rem; }
      .motor-controls { flex-direction: column; }
      button { font-size: 0.9rem; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Motor Control Remote</h1>
    <div>Status: <span id="status" class="disconnected">Disconnected</span></div>
    <div id="messages"></div>
    <div class="control-group">
      <div class="motor-controls">
        <div class="motor-box">
          <h2>Motor A</h2>
          <button class="forward-btn" onclick="sendCommand('motorA_forward')">Forward</button>
          <button class="backward-btn" onclick="sendCommand('motorA_backward')">Backward</button>
          <button class="stop-btn" onclick="sendCommand('motorA_stop')">Stop</button>
        </div>
        <div class="motor-box">
          <h2>Motor B</h2>
          <button class="forward-btn" onclick="sendCommand('motorB_forward')">Forward</button>
          <button class="backward-btn" onclick="sendCommand('motorB_backward')">Backward</button>
          <button class="stop-btn" onclick="sendCommand('motorB_stop')">Stop</button>
        </div>
      </div>
    </div>
  </div>

  <script>
    const socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    const statusEl = document.getElementById('status');
    const messagesEl = document.getElementById('messages');
    
    socket.onopen = () => {
      statusEl.textContent = 'Connected';
      statusEl.className = 'connected';
      addMessage('[System] WebSocket connected', 'system');
    };
    
    socket.onmessage = (e) => {
      addMessage(e.data, 'motor');
    };
    
    socket.onclose = () => {
      statusEl.textContent = 'Disconnected';
      statusEl.className = 'disconnected';
      addMessage('[System] Connection lost. Refresh page to reconnect.', 'system');
    };
    
    function sendCommand(command) {
      if(socket.readyState === WebSocket.OPEN) {
        socket.send(command);
      }
    }
    
    function addMessage(msg, type) {
      const div = document.createElement('div');
      div.className = `message ${type}`;
      div.textContent = msg;
      messagesEl.appendChild(div);
      messagesEl.scrollTo({ top: messagesEl.scrollHeight, behavior: 'smooth' });
    }
  </script>
</body>
</html>
)=====";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Motor Control with WebSocket");

  // Set motor control pins as outputs
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorB_IN3, OUTPUT);
  pinMode(motorB_IN4, OUTPUT);
  pinMode(nSLEEP_PIN, OUTPUT);
  digitalWrite(nSLEEP_PIN, HIGH); // Enable the driver

  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // Start Web Server
  server.on("/", handleRoot);
  server.begin();

  // Start WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.println("Ready! Control motors via web interface");
}

void loop() {
  webSocket.loop();
  server.handleClient();
  
  // Read from Serial and send to Web
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

// --- Motor A Control Functions ---
void moveMotorA_Forward() {
  digitalWrite(motorA_IN1, HIGH);
  digitalWrite(motorA_IN2, LOW);
  Serial.print("Initial Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

void moveMotorA_Backward() {
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, HIGH);
  Serial.print("Initial Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

void stopMotorA() {
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, LOW);
  Serial.print("Initial Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

// --- Motor B Control Functions ---
void moveMotorB_Forward() {
  digitalWrite(motorB_IN3, HIGH);
  digitalWrite(motorB_IN4, LOW);
  Serial.print("Initial Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

void moveMotorB_Backward() {
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, HIGH);
  Serial.print("Initial Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

void stopMotorB() {
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, LOW);
  Serial.print("Initial Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}