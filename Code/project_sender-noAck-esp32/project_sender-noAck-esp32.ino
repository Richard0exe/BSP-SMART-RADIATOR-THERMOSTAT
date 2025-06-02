#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>
#include "Communications.h"
#include "Messages.h"
#include "RadiatorManager.h"

#define DEBUG FALSE // CHANGE TO TRUE TO ENABLE SERIAL OUTPUTS 

#define ESPNOW_CHANNEL 6

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define DHT_PIN 13 // D13
#define DHT_TYPE DHT11 
#define BUTTON_PIN 12 // D12

#define RX2 16
#define TX2 17

#define SDA_PIN 21  // Define SDA pin (GPIO 21)
#define SCL_PIN 22  // Define SCL pin (GPIO 22)

#define ENCODER_CLK 27  // D5
#define ENCODER_DT  26  // D6
#define ENCODER_SW  14  // D0 (reuses your button)

Communications coms;
RadiatorManager radiatorManager(coms);

uint8_t commonTemp = DEFAULT_TEMP;
uint8_t rotatorTemp = commonTemp;
uint8_t shownTemp = rotatorTemp;
bool tempChanged = false;

String PARAM_MESSAGE = "temperature";

const unsigned char check_icon [] PROGMEM = {
  0b00000000,
  0b00000001,
  0b00000011,
  0b00000110,
  0b10001100,
  0b11011000,
  0b01110000,
  0b00100000
};

const unsigned char cross_icon [] PROGMEM = {
  0b10000001,
  0b01000010,
  0b00100100,
  0b00011000,
  0b00011000,
  0b00100100,
  0b01000010,
  0b10000001
};

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

int prev_button_state = HIGH;
int button_state;

// -1 - all, 0-n - all radiators in radiators array
int currentRadiatorIndex = -1;

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
  // Initialize the DHT sensor
  dht.begin();
  
  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // Initialize communications
  coms.begin();
  coms.setName("server");

  coms.setReceiveHandler(OnDataRecv);
  coms.setDiscoveryHandler(OnDiscoverNewPeer);

  coms.broadcastDiscovery();
}

void sendRadiatorsToWeb(){
  const Radiator* radiators = radiatorManager.getRadiators();
  StaticJsonDocument<768> doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < radiatorManager.getNumRadiators(); i++) {
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
  serializeJson(doc, Serial2);  // Or to SoftwareSerial
  Serial2.println(); // newline to indicate end
}

//process received json and update the radiators struct array
void processJSON(String jsonStr){

}

static uint8_t lastSentTemperature = 0;
static unsigned long lastSendTime = 0;
static unsigned long lastWebSendTime = 0;
const uint16_t sendInterval = 10000; // 10 seconds delay

int lastEncoderButtonState = HIGH;
int lastEncoderState = HIGH;
int encoderClicked = false;

bool buttonClicked = false;

String readSerial2Line() {
  static String buffer = "";
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      String line = buffer;
      buffer = "";
      return line;
    } else {
      buffer += c;
    }
  }
  return "";  // Nothing complete yet
}


void readFromWebServer() {
  String incoming = readSerial2Line();
  if (incoming.length() > 0) {
    Serial.print("Received from WEB: ");
    Serial.println(incoming);

    // Handle command
    if (incoming.startsWith("ALL/T")) {
      String tempStr = incoming.substring(5); // Extract "23" from "ALL/T23"
      radiatorManager.sendTemperatureToAll(tempStr.toInt());
    }
  }
}


void loop() {
// //  constantly reading Serial2 waiting for some info
//   readFromWebServer();

// // send some info to webserver every time n period
//   if((millis() - lastWebSendTime) > sendInterval){
//     sendRadiatorsToWeb();
//     lastWebSendTime = millis();
//   }

  button_state = digitalRead(BUTTON_PIN);

// External button pess logic
  if(button_state == LOW && prev_button_state == HIGH){
    delay(50);
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

int encoderButtonState = digitalRead(ENCODER_SW);
if(encoderButtonState == LOW && lastEncoderButtonState == HIGH){
  encoderClicked = true;
} else {
  encoderClicked = false;
}
lastEncoderButtonState = encoderButtonState;

int currentState = digitalRead(ENCODER_CLK);
if(currentState != lastEncoderState && currentState == LOW){
  int delta = (digitalRead(ENCODER_DT) != currentState) ? 1 : -1;
  rotatorTemp = constrain(rotatorTemp + delta, MIN_TEMP, MAX_TEMP);
  shownTemp = rotatorTemp;
  }
lastEncoderState = currentState;

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

  // Clear display
  display.clearDisplay();
  
  //if 1 from all radiators dont confirm receiving, print CROSS
  bool allAcks = true;
  if (currentRadiatorIndex == -1) {
  for (int i = 0; i < radiatorManager.getNumRadiators(); i++) {
    if (!radiatorManager.isAcked(i)) {
      allAcks = false;
      break;
    }
  }
} else {
  if (!radiatorManager.isAcked(currentRadiatorIndex)) {
    allAcks = false;
  }
}

    // Show icon
  if(allAcks){
    display.drawBitmap(0,0, check_icon, 8, 8, WHITE);
  } else {
    display.drawBitmap(0,0, cross_icon, 8, 8, WHITE);
  }

  //display choosen radiator
  display.setTextSize(1);
  display.setCursor(0, 24);
  if (currentRadiatorIndex == -1) {
    display.print("All");
  } else {
    display.print(radiatorManager.getRadiatorName(currentRadiatorIndex));
  }

  // Display potentiometer-based temperature
  display.setTextSize(3);
  display.setCursor(48, 0);
  display.print(shownTemp);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);  // Degree symbol
  display.setTextSize(2);
  display.print("C");

  // Display DHT11 temperature
  display.setTextSize(1);
  display.setCursor(85, 24);
  display.print("T: ");
  display.print(temp, 0);
  display.cp437(true);
  display.write(167);  // Degree symbol
  display.print("C ");

  display.display();
}