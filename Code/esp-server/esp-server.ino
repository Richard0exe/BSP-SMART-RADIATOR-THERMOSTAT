#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>
#include "Communications.h" 
#include "Messages.h"
#include "RadiatorManager.h"
#include "RadiatorDisplay.h"
#include "WebComs.h"
#include "Button.h"

#define DEBUG FALSE // CHANGE TO TRUE TO ENABLE SERIAL OUTPUTS 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define DHT_PIN 13 // D13
#define DHT_TYPE DHT11 
#define BUTTON_PIN 12 // D12
#define INFO_BUTTON_PIN 25 // D25

#define RX2 16
#define TX2 17

#define SDA_PIN 21  // Define SDA pin (GPIO 21)
#define SCL_PIN 22  // Define SCL pin (GPIO 22)

#define ENCODER_CLK 27  // D5
#define ENCODER_DT  26  // D6
#define ENCODER_SW  14  // D0 (reuses your button)

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

Communications coms;
RadiatorManager radiatorManager(coms);
RadiatorDisplay radiatorDisplay(display);
WebComs webComs(Serial2, radiatorManager);

Button infoButton(INFO_BUTTON_PIN);

void OnDataRecv(const uint8_t* mac, uint8_t type, const uint8_t* data, int len){
  switch (type) {
    case MSG_TYPE_TEMPERATURE_RESPONSE:
      if (len == sizeof(TemperatureResponse)) {
        TemperatureResponse payload;
        memcpy(&payload, data, sizeof(payload));
        radiatorManager.processTemperatureResponse(mac, payload);
      }
      break;

    default:
      Serial.println("Unknown message type");
      break;
  }
}

void OnDiscoverNewPeer(const Peer& peer) {
  // Add new radiator
  if (strncmp(peer.name, "radiator", MAX_NAME_LEN) == 0) {
    radiatorManager.handleDiscovery(peer);
  } else {
    Serial.printf("Discovered unknown type device: %s (%s)\n", peer.name, Communications::macToString(peer.mac).c_str());
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX2, TX2);

  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  infoButton.begin();
  // Initialize the DHT sensor
  dht.begin();
  
  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  radiatorDisplay.begin();
  
  // Initialize communications
  coms.begin();
  coms.setName("server");

  coms.setReceiveHandler(OnDataRecv);
  coms.setDiscoveryHandler(OnDiscoverNewPeer);

  coms.broadcastDiscovery();
}

//-- Radiator selection satate
int currentRadiatorIndex = -1; // -1 - all, 0-n - all radiators in radiators array
uint8_t commonTemp = DEFAULT_TEMP;
uint8_t rotatorTemp = commonTemp;
uint8_t shownTemp = rotatorTemp;
bool tempChanged = false;

//-- Button and encoder states
int prev_button_state = HIGH;
int button_state;
bool buttonClicked = false;

int lastEncoderButtonState = HIGH;
int lastEncoderState = HIGH;
int encoderClicked = false;

//-- UI States

enum UI_State : uint8_t {
  UI_RADIATORS,
  UI_INFO
};
UI_State state = UI_RADIATORS;
bool changed = false;

void radiatorState(bool redraw = false) {
  if (redraw) {
    radiatorDisplay.redraw();
  }

    // External button pess logic
  button_state = digitalRead(BUTTON_PIN);
  if(button_state == LOW && prev_button_state == HIGH){
    delay(50); // maybe there is a nonblocking way to do this?
    buttonClicked = true;

    currentRadiatorIndex++;
    if(currentRadiatorIndex >= radiatorManager.getNumRadiators()){
      currentRadiatorIndex = -1;
    }

    Serial.print("Selected: ");
    if (currentRadiatorIndex == -1) {
      shownTemp = commonTemp;
      Serial.println("ALL radiators");
    } else {
      shownTemp = radiatorManager.getRadiatorTemperature(currentRadiatorIndex);
      Serial.println(radiatorManager.getRadiatorName(currentRadiatorIndex));
    }

    rotatorTemp = shownTemp; 
  }

  prev_button_state = button_state;

  // Encoder button logic
  int encoderButtonState = digitalRead(ENCODER_SW);
  if(encoderButtonState == LOW && lastEncoderButtonState == HIGH){
    encoderClicked = true;
  } else {
    encoderClicked = false;
  }
  lastEncoderButtonState = encoderButtonState;

  // Rotator logic
  int currentState = digitalRead(ENCODER_CLK);
  if(currentState != lastEncoderState && currentState == LOW){
    int delta = (digitalRead(ENCODER_DT) != currentState) ? 1 : -1;
    rotatorTemp = constrain(rotatorTemp + delta, MIN_TEMP, MAX_TEMP);
    shownTemp = rotatorTemp;
  }
  lastEncoderState = currentState;

  // Encoder button logic
  if(encoderClicked){
    if (currentRadiatorIndex == -1) {
      // Send to all radiators
      commonTemp = rotatorTemp;
      radiatorManager.sendTemperatureToAll(commonTemp);
    } else {
      // Send only to selected radiator
      radiatorManager.sendTemperatureTo(currentRadiatorIndex, rotatorTemp);
    }

    buttonClicked = false;
  }

  // Read DHT sensor values
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(temp) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Serial.print("Temperature: ");
    // Serial.print(temp);
    // Serial.println("°C");
    // Serial.print("Humidity: ");
    // Serial.print(humidity);
    // Serial.println("%");
  }

  // display logic
  //if 1 from all radiators dont confirm receiving, print CROSS
  bool acked = currentRadiatorIndex == -1 ? radiatorManager.isAllAcked() : radiatorManager.isAcked(currentRadiatorIndex);
  //display choosen radiator
  String name = currentRadiatorIndex == -1 ? "All" : radiatorManager.getRadiatorName(currentRadiatorIndex);
  // If anything has changed then it will update the display
  radiatorDisplay.update(currentRadiatorIndex, name, shownTemp, acked, temp);
}

void infoState(bool redraw) {
  if (!redraw) return; // avoid redrawing

  display.clearDisplay();

  display.setTextSize(1);

  // Display IP
  display.setCursor(0, 0);
  display.print("IP: ");
  display.println(webComs.ip);

  // Display SSID
  display.setCursor(0, 10);
  display.print("SSID: ");
  display.println(webComs.ssid);

  // Display Password
  display.setCursor(0, 20);
  display.print("PWD: ");
  display.println(webComs.password);

  display.display();
}

void loop() {
  // constantly reading Serial2 waiting for some info
  webComs.update();

  infoButton.update();
  if (infoButton.wasPressed()) {
    state = (state == UI_INFO) ? UI_RADIATORS : UI_INFO;
    changed = true;
  } else {
    changed = false;
  }

  switch (state) {
    case UI_RADIATORS:
      radiatorState(changed);
      break;
    case UI_INFO:
      infoState(changed);
      break;
  }
}
