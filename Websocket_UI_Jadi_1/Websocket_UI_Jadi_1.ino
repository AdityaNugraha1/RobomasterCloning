#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

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

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 WebSerial Gateway</title>
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
    }
    .connected { color: #16a34a; }
    .disconnected { color: #dc2626; }
    #messages {
      height: 300px;
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
    .serial { background: #ecfdf5; color: #16a34a; }
    .user { background: #eff6ff; color: #2563eb; }
    .system { color: #6b7280; }
    .input-group {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
    }
    #input {
      flex: 1;
      padding: 10px;
      border: 1px solid #d1d5db;
      border-radius: 5px;
      font-size: 1rem;
      outline: none;
    }
    #input:focus {
      border-color: #2563eb;
      box-shadow: 0 0 0 2px rgba(37,99,235,0.2);
    }
    button {
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      font-size: 1rem;
      cursor: pointer;
      transition: background 0.2s;
    }
    .send-btn { background: #2563eb; color: #fff; }
    .send-btn:hover { background: #1d4ed8; }
    .clear-btn { background: #dc2626; color: #fff; }
    .clear-btn:hover { background: #b91c1c; }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    @media (max-width: 600px) {
      .container { padding: 15px; }
      h1 { font-size: 1.25rem; }
      #input, button { font-size: 0.9rem; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 WebSerial Gateway</h1>
    <div>Status: <span id="status" class="disconnected">Disconnected</span></div>
    <div id="messages"></div>
    <div class="input-group">
      <input id="input" type="text" placeholder="Type message...">
      <button class="send-btn" onclick="sendMessage()">Send</button>
      <button class="clear-btn" onclick="clearMessages()">Clear</button>
    </div>
  </div>

  <script>
    const socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    const statusEl = document.getElementById('status');
    const messagesEl = document.getElementById('messages');
    const inputEl = document.getElementById('input');
    
    socket.onopen = () => {
      statusEl.textContent = 'Connected';
      statusEl.className = 'connected';
      addMessage('[System] WebSocket connected', 'system');
    };
    
    socket.onmessage = (e) => {
      if(e.data.startsWith('[Serial]')) {
        addMessage(e.data, 'serial');
      }
    };
    
    socket.onclose = () => {
      statusEl.textContent = 'Disconnected';
      statusEl.className = 'disconnected';
      addMessage('[System] Connection lost. Refresh page to reconnect.', 'system');
    };
    
    function sendMessage() {
      const msg = inputEl.value.trim();
      if(msg && socket.readyState === WebSocket.OPEN) {
        socket.send(msg);
        addMessage('[You] ' + msg, 'user');
        inputEl.value = '';
      }
    }
    
    function clearMessages() {
      messagesEl.innerHTML = '';
    }
    
    function addMessage(msg, type) {
      const div = document.createElement('div');
      div.className = `message ${type}`;
      div.textContent = msg;
      messagesEl.appendChild(div);
      messagesEl.scrollTo({ top: messagesEl.scrollHeight, behavior: 'smooth' });
    }
    
    inputEl.addEventListener('keypress', (e) => {
      if(e.key === 'Enter') sendMessage();
    });
  </script>
</body>
</html>
)=====";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // Start Web Server
  server.on("/", handleRoot);
  server.begin();

  // Start WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.println("Ready! Send messages via web interface or Serial Monitor");
}

void loop() {
  webSocket.loop();
  server.handleClient();
  
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