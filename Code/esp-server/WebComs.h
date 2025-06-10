#ifndef WEBCOMS_H
#define WEBCOMS_H

#include "RadiatorManager.h"

class WebComs {
public:
    String ssid;
    String password;
    String ip;

    WebComs(HardwareSerial& serial, RadiatorManager& manager);
    void update();

private:
    HardwareSerial& _serial;
    RadiatorManager& _manager;
    String _buffer;

    void handleLine(const String& line);
    void sendRadiatorStates();
    int splitString(const String& str, char delimiter, String* parts, int maxParts);
};


#endif