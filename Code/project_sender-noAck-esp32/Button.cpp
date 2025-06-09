#include <Arduino.h>
#include "Button.h"

Button::Button(uint8_t pin) : pin(pin) {}

void Button::begin() {
  pinMode(pin, INPUT_PULLUP);
}

void Button::update() {
// read the state of the switch into a local variable:
  int reading = digitalRead(pin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        pressedFlag = true; // mark as just pressed
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}

bool Button::isPressed() {
  return buttonState == LOW;
}

bool Button::wasPressed() {
  if (pressedFlag) {
    pressedFlag = false;
    return true;
  }

  return false;
}

