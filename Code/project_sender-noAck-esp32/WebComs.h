#ifndef WEBCOMS_H
#define WEBCOMS_H

#include "RadiatorManager.h"

class WebComs {
public:
    WebComs(HardwareSerial& serial, RadiatorManager& manager);
    void update();

private:
    HardwareSerial& _serial;
    RadiatorManager& _manager;
    String _buffer;

    void handleLine(const String& line);
    void sendRadiatorStates();
};


#endif