#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

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

#define MIN_TEMP 8
#define MAX_TEMP 28

#define DEFAULT_TEMP 20

uint8_t commonTemp = DEFAULT_TEMP;
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

// Set the MAC address of the receiver (replace with the actual MAC address)
uint8_t receiverMacAddress[] = {0x8C, 0x4F, 0x00, 0x42, 0x02, 0x25}; // Your MAC address

// Data structure to send via ESP-NOW
typedef struct struct_message {
  uint8_t msgType; // Now only type DATA
  uint16_t msgId;
  uint8_t temp; 
} struct_message;

typedef struct { 
  uint8_t mac[6];
  char name[16];
  uint8_t curr_temp; // hold current temp for each radiator
  bool ackReceived; 
} Radiator;

Radiator radiators[] = {
  { {0x98, 0x88, 0xE0, 0xD9, 0xCB, 0x18}, "Room 1", DEFAULT_TEMP},
  { {0x8C, 0x4F, 0x00, 0x42, 0x02, 0x28}, "Room 2", DEFAULT_TEMP}, // some fake MAC for testing 
};

const int numRadiators = sizeof(radiators) / sizeof(radiators[0]);

// -1 - all, 0-n - all radiators in radiators array
int currentRadiatorIndex = -1;

struct_message myData;

static uint16_t lastMsgId  = 0;

enum MessageType {
  DATA = 0,
  ACK = 1
  };
MessageType messageType;

// Setup ESP-NOW communication
void OnDataSent(const unsigned char *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.print("Send Status to ");

  //Find to which radiator data was sent succesfully
  bool found = false;
  for(int i = 0; i < numRadiators; i++){
    // compare to memory blocks byte by byte , MAC address = 6 bytes 
    if(memcmp(mac_addr, radiators[i].mac, 6) == 0){
      Serial.print((radiators[i].name));
      found = true;

      // Update ackReceived based on send status
      if (sendStatus == ESP_NOW_SEND_SUCCESS) {
        radiators[i].ackReceived = true;  // Success
      } else {
        radiators[i].ackReceived = false; // Failure
      }
      break;
    }
  }
  if(!found) {
    Serial.print("Unknown device");
  }

  Serial.print(": "); 
  if (sendStatus == 0) {
    Serial.println("Success");
  } else {
     Serial.println("Fail");
  }
}

// void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len){
//   struct_message incomingMsg;
//   memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));

//   if(incomingMsg.msgType == ACK){
//     for (int i = 0; i < numRadiators; i++) {
//       if (memcmp(mac, radiators[i].mac, 6) == 0 && incomingMsg.msgId == lastMsgId) {
//         radiators[i].ackReceived = true;
//         Serial.print("ACK received from ");
//         Serial.println(radiators[i].name);
//       } 
//     }
//   }
// }

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
  
  // Initialize Wi-Fi for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Ensure Wi-Fi is off
  //WiFi.channel(ESPNOW_CHANNEL);
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  //esp_now_register_recv_cb(OnDataRecv);
  
  // Add the receiver as a peer
 for (int i = 0; i < numRadiators; i++) {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, radiators[i].mac, 6);
  //peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  
  esp_now_add_peer(&peerInfo);
}
}

// bool waitForAck(Radiator &targetRadiator, unsigned long timeoutMs = 1000, uint8_t maxRetries = 3){
//   int retries = 0;

//   while(retries < maxRetries){
//   unsigned long start = millis();
//   targetRadiator.ackReceived = false;

//   while(!targetRadiator.ackReceived && (millis() - start < timeoutMs)) {
//     delay(1);
//   }

//   if (!targetRadiator.ackReceived) {
//     Serial.println("ACK not received (timeout)");
//     retries++;
//   } else {
//     Serial.println("ACK received");
//     return true;
//   }
//   }
//   // After max retries
//   Serial.println("Failed to receive ACK after retries.");
//   return false;
// }

void setALLTemperature(String temperature){
  int temp = temperature.toInt();

  // Check if the temperature is within the valid range
  if (temp >= MIN_TEMP && temp <= MAX_TEMP) {
    // Also update the common temperature
    commonTemp = temp;
    tempChanged = true;
    // Debug print to show that temperature has been updated
    Serial.print("Set temperature to: ");
    Serial.println(temp);
  }
  return;
}

void sendRadiatorsToWeb(){
  StaticJsonDocument<768> doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < numRadiators; i++) {
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

void loop() {

//constantly reading Serial2 waiting for some info
  while (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    Serial.print("Received from WEB: ");
    Serial.println(incoming);

    // Check if it starts with "ALL/T"
    if (incoming.startsWith("ALL/T")) {
    String tempStr = incoming.substring(5); // Extract "23" from "ALL/T23"
    setALLTemperature(tempStr);
    }
  }

// send some info to webserver every time n period
  if((millis() - lastWebSendTime) > sendInterval){
    sendRadiatorsToWeb();
    lastWebSendTime = millis();
  }

  button_state = digitalRead(BUTTON_PIN);

// External button pess logic
  if(button_state == LOW && prev_button_state == HIGH){
    delay(50);
    buttonClicked = true;

    currentRadiatorIndex++;
    if(currentRadiatorIndex >= numRadiators){
      currentRadiatorIndex = -1;
    }

    Serial.print("Selected: ");
    if (currentRadiatorIndex == -1) {
      Serial.println("ALL radiators");
    } else {
      Serial.println(radiators[currentRadiatorIndex].name);
    }
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

  if(currentRadiatorIndex == -1){
    commonTemp = constrain(commonTemp + delta, MIN_TEMP, MAX_TEMP);
   // Apply to all radiators
    for (int i = 0; i < numRadiators; i++) {
      radiators[i].curr_temp = commonTemp;
    }
  } else {
    radiators[currentRadiatorIndex].curr_temp = constrain(radiators[currentRadiatorIndex].curr_temp + delta, MIN_TEMP, MAX_TEMP);
  }
  }
lastEncoderState = currentState;

if (tempChanged) {
  // Temperature was changed via Web UI
  tempChanged = false;

  // Apply temperature to all radiators
  for (int i = 0; i < numRadiators; i++) {
    radiators[i].curr_temp = commonTemp;
    radiators[i].ackReceived = false;

    lastMsgId++;
    if (lastMsgId == 0) lastMsgId = 1;

    myData.msgType = DATA;
    myData.msgId = lastMsgId;
    myData.temp = commonTemp;

    esp_now_send(radiators[i].mac, (uint8_t *)&myData, sizeof(myData));
  }

  lastSendTime = millis();
}

//if(((millis() - lastSendTime) > sendInterval) || encoderClicked){ use to send between time interval
if(encoderClicked){
  if (currentRadiatorIndex == -1) {
  for (int i = 0; i < numRadiators; i++) {
    radiators[i].ackReceived = false;
  }
  } else {
    radiators[currentRadiatorIndex].ackReceived = false;
  }

    if (currentRadiatorIndex == -1) {
    // Send to all radiators
    for (int i = 0; i < numRadiators; i++) {
      lastMsgId++;
      if (lastMsgId == 0) lastMsgId = 1; // avoid 0

      myData.msgType = DATA;
      myData.msgId = lastMsgId;
      myData.temp = commonTemp;

      esp_now_send(radiators[i].mac, (uint8_t *)&myData, sizeof(myData));
      //waitForAck(radiators[i]);
    }
  } else {
    // Send only to selected radiator
    lastMsgId++;
    if (lastMsgId == 0) lastMsgId = 1;

    myData.msgType = DATA;
    myData.msgId = lastMsgId;
    myData.temp = radiators[currentRadiatorIndex].curr_temp;

    esp_now_send(radiators[currentRadiatorIndex].mac, (uint8_t *)&myData, sizeof(myData));
    //waitForAck(radiators[currentRadiatorIndex]);
  }

  lastSendTime = millis();
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
  for (int i = 0; i < numRadiators; i++) {
    if (!radiators[i].ackReceived) {
      allAcks = false;
      break;
    }
  }
} else {
  if (!radiators[currentRadiatorIndex].ackReceived) {
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
    display.print(radiators[currentRadiatorIndex].name);
  }

  // Display potentiometer-based temperature
  uint8_t shownTemp = (currentRadiatorIndex == -1) ? commonTemp : radiators[currentRadiatorIndex].curr_temp;
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
