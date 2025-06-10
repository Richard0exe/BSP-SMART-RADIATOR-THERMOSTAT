#ifndef RADIATOR_DISPLAY_H
#define RADIATOR_DISPLAY_H

#include <Adafruit_SSD1306.h>

class RadiatorDisplay {
public:
  RadiatorDisplay(Adafruit_SSD1306& display);

  void begin();
  void update(int radiatorIndex, const String& name, uint8_t shownTemp, bool ackReceived, float dhtTemp);
  void redraw();

private:
  Adafruit_SSD1306& display;

  // Last rendered state
  int lastRadiatorIndex = -99; // Invalid initial value
  String lastName = "";
  uint8_t lastShownTemp = 255; // Invalid temp
  bool lastAck = false;
  float lastDhtTemp = -1000.0; // Definitely out of range

  void render(const String& name, uint8_t shownTemp, bool ackReceived, float dhtTemp);
  void drawAckIcon(bool ackReceived);
  void drawRadiatorName(const String& name);
  void drawShownTemp(uint8_t temp);
  void drawDhtTemp(float temp);
};

#endif