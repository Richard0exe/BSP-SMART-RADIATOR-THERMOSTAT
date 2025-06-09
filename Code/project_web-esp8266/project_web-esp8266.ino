#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define RX 4 //D2
#define TX 5 //D1

SoftwareSerial communicationSerial(RX, TX);

const char *ssid = "ESP32-Access-Point";
const char *password = "123456789";

typedef struct { 
  uint8_t mac[6];
  char name[16];
  uint8_t curr_temp; // hold current temp for each radiator
  bool ackReceived; 
} Radiator;

//define websocket with url /ws
AsyncWebSocket ws("/ws");
AsyncWebServer server(80);

String PARAM_MESSAGE = "temperature";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void sendTemperature(String temperature) {
  int temp = temperature.toInt();
  if (temp >= 8 && temp <= 28) {
    Serial.print("Set temperature to: ");
    Serial.println(temp);
    //Sending flag to set temperature to all
    communicationSerial.print("ALL/T/");
    communicationSerial.println(temp);
  }
}

void sendInfo() {
  String IP = WiFi.softAPIP().toString();

  communicationSerial.print("INFO/");
  communicationSerial.print(IP);
  communicationSerial.print("/");
  communicationSerial.print(ssid);
  communicationSerial.print("/");
  communicationSerial.print(password);
  communicationSerial.println();
}


void setup() {
  Serial.begin(115200);

  communicationSerial.begin(9600);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  Serial.println("Starting LittleFS!");
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  Dir dir = LittleFS.openDir("/");
  Serial.println("Files:");
  while (dir.next()) {
    Serial.print("File: ");
    Serial.println(dir.fileName());
  }
  Serial.println("All files listed");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "text/javascript");
  });

  server.on("/edit.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/edit.svg", "image/svg+xml");
  });

  server.on("/delete.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/delete.svg", "image/svg+xml");
  });

  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    String temperature;
    if (request->hasParam(PARAM_MESSAGE)) {
      temperature = request->getParam(PARAM_MESSAGE)->value();
      Serial.print("Received temperature: ");
      Serial.println(temperature);
      if (temperature.length() > 0) {
        sendTemperature(temperature);
      }
    } else {
      temperature = "No message sent";
    }
    request->send(200, "text/plain", "Received temperature: " + temperature);
  });

  server.onNotFound(notFound);
  server.begin();
// socket handler which listens for client connection
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, 
  AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    communicationSerial.println("GET/RADIATORS");
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
  });
  server.addHandler(&ws);

  sendInfo(); // send IP, ssid, password to esp-server
}

String incomingJSON = "";

void handleSerialInput() {
  while (communicationSerial.available()) {
    char c = communicationSerial.read();
    //Serial.print(c);
    if (c == '\n') {
      //Serial.println();
      sendToWeb(incomingJSON);
      incomingJSON = "";
    } else {
      incomingJSON += c;
    }
  }
}

//send unchanged json to web
void sendToWeb(String jsonStr) {
  Serial.println(jsonStr);
  ws.textAll(jsonStr);
}

void loop() {
  // listen communicationSerial for incoming messages from ESP32, radiator lists updates and some other info (battery maybe)
  handleSerialInput();
}
