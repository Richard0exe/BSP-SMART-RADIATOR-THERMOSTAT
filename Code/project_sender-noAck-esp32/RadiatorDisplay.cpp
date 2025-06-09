#include "RadiatorDisplay.h"

static const unsigned char check_icon [] PROGMEM = {
  0b00000000,
  0b00000001,
  0b00000011,
  0b00000110,
  0b10001100,
  0b11011000,
  0b01110000,
  0b00100000
};

static const unsigned char cross_icon [] PROGMEM = {
  0b10000001,
  0b01000010,
  0b00100100,
  0b00011000,
  0b00011000,
  0b00100100,
  0b01000010,
  0b10000001
};

RadiatorDisplay::RadiatorDisplay(Adafruit_SSD1306& disp) : display(disp) {}

void RadiatorDisplay::begin() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
}

void RadiatorDisplay::update(int radiatorIndex, const String& name, uint8_t shownTemp, bool ackReceived, float dhtTemp) {
  if (shownTemp == lastShownTemp &&
      radiatorIndex == lastRadiatorIndex &&
      name == lastName &&
      ackReceived == lastAck &&
      abs(dhtTemp - lastDhtTemp) < 0.5) {
    return; // No change
  }

  lastName = name;
  lastShownTemp = shownTemp;
  lastAck = ackReceived;
  lastDhtTemp = dhtTemp;

  render(name, shownTemp, ackReceived, dhtTemp);
}

void RadiatorDisplay::redraw() {
  render(lastName, lastShownTemp, lastAck, lastDhtTemp);
}

void RadiatorDisplay::render(const String& name, uint8_t shownTemp, bool ackReceived, float dhtTemp) {
  display.clearDisplay();

  drawAckIcon(ackReceived);
  drawRadiatorName(name);
  drawShownTemp(shownTemp);
  drawDhtTemp(dhtTemp);

  display.display();
}

void RadiatorDisplay::drawAckIcon(bool ackReceived) {
  const unsigned char* icon = ackReceived ? check_icon : cross_icon;
  display.drawBitmap(0, 0, icon, 8, 8, WHITE);
}

void RadiatorDisplay::drawRadiatorName(const String& name) {
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print(name);
}

void RadiatorDisplay::drawShownTemp(uint8_t temp) {
  display.setTextSize(3);
  display.setCursor(48, 0);
  display.print(temp);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);  // Degree symbol
  display.setTextSize(2);
  display.print("C");
}

void RadiatorDisplay::drawDhtTemp(float temp) {
  display.setTextSize(1);
  display.setCursor(85, 24);
  display.print("T: ");
  display.print(temp, 0);
  display.cp437(true);
  display.write(167);  // Degree symbol
  display.print("C ");
}
