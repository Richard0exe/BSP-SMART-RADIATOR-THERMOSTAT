#include "WebComs.h"
#include <ArduinoJson.h>

WebComs::WebComs(HardwareSerial& serial, RadiatorManager& manager)
  : _serial(serial), _manager(manager), _buffer("") {}

void WebComs::update() {
  while (_serial.available()) {
    char c = _serial.read();
    if (c == '\n') {
      handleLine(_buffer);
      _buffer = "";
    } else {
      _buffer += c;
    }
  }
}

void WebComs::handleLine(const String& lineRaw) {
  String line = lineRaw;
  line.trim();  // Removes any \r, spaces, etc.
  Serial.print("Received command: ");
  Serial.println(line);

  const int MAX_PARTS = 5;
  String parts[MAX_PARTS];
  int numParts = splitString(line, '/', parts, MAX_PARTS);

  if (numParts == 0) return; // nothing to do

  if (parts[0] == "ALL" && parts[1] == "T" && numParts >= 3) {
      int temp = parts[2].toInt();
      _manager.sendTemperatureToAll(temp);
  } else if (parts[0] == "GET" && parts[1] == "RADIATORS") {
      sendRadiatorStates();
  } else if (parts[0] == "INFO") {
    ip = parts[1];
    ssid = parts[2];
    password = parts[3];
  } else if (parts[0] == "SET") {
    if (parts[1] == "TEMP") { // SET/TEMP/<id>/<temperature>
      int index = parts[2].toInt();
      int temp = parts[3].toInt();
      Serial.printf("Setting temperature to [%d]: %d°C\n", index, temp);
      _manager.sendTemperatureTo(index, temp);
    }
  }

  // other possible commands
  // SET/ID/TEMP/23 // sets temperature for radiator
  // SET/ID/NAME/NewName // sets new name for radiator
  // GET/ID // gets specific radiator
  // maybe also change ALL/T23 to SET/ALL/TEMP/23
}

void WebComs::sendRadiatorStates() {
  const Radiator* radiators = _manager.getRadiators();
  StaticJsonDocument<768> doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < _manager.getNumRadiators(); i++) {
    JsonObject obj = arr.createNestedObject();

    char macStr[18];
    //make desired mac string using sprintf
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            radiators[i].mac[0], radiators[i].mac[1], radiators[i].mac[2],
            radiators[i].mac[3], radiators[i].mac[4], radiators[i].mac[5]);
    
    obj["mac"] = macStr;
    obj["name"] = radiators[i].name;
    obj["curr_temp"] = radiators[i].curr_temp;
    obj["ack"] = radiators[i].ackReceived;
  }

  Serial.println("Sending radiators JSON");
  serializeJson(doc, _serial);  // Or to SoftwareSerial
  _serial.println(); // newline to indicate end
}

int WebComs::splitString(const String& str, char delimiter, String* parts, int maxParts) {
    int partCount = 0;
    int start = 0;
    int end = str.indexOf(delimiter);

    while (end != -1 && partCount < maxParts - 1) {
        parts[partCount++] = str.substring(start, end);
        start = end + 1;
        end = str.indexOf(delimiter, start);
    }

    // Add the last part
    if (partCount < maxParts) {
        parts[partCount++] = str.substring(start);
    }

    return partCount;
}
