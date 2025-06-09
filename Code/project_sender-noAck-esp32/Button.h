#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
public:
    Button(uint8_t pin);
    void begin();
    void update();
    bool wasPressed();
    bool isPressed();

private:
    uint8_t pin;
    int buttonState = LOW;
    int lastButtonState = HIGH;
    unsigned long debounceDelay = 50;
    unsigned long lastDebounceTime = 0;

    bool pressedFlag = false;
    bool releasedFlag = false;
};


#endif