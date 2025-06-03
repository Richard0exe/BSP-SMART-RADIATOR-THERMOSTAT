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

  if (line.startsWith("ALL/T")) {
    int temp = line.substring(5).toInt(); // Extract "23" from "ALL/T23"
    _manager.sendTemperatureToAll(temp);
  } else if (line == "GET/RADIATORS") {
    sendRadiatorStates();
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
