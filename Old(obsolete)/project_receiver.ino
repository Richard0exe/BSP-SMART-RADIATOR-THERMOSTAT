#include <ESP8266WiFi.h>
#include <espnow.h>
#include <AccelStepper.h>

#define DEBUG FALSE // CHANGE TO TRUE TO ENABLE SERIAL OUTPUTS 
#define ESPNOW_CHANNEL 6
#define STEPS_PER_REVOLUTION 2048

#define IN1 14 // D5
#define IN2 5 // D1
#define IN3 12 // D6
#define IN4 13 // D7

const int stepsPerRevolution = STEPS_PER_REVOLUTION; // For 28BYJ-48 motor

AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

// Structure to receive data (temperature)
typedef struct struct_message {
  uint8_t msgType;
  uint16_t msgId;
  uint8_t temp;  // Temperature value received
} struct_message;

struct_message myData;

enum MessageType {
  DATA = 0,
  ACK = 1
  };
MessageType messageType;

// Callback function that wilal be executed when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  if(myData.msgType == DATA){
  Serial.println("Message type: DATA ");

  sendAck(mac, myData.msgId);

  Serial.print("Received Temperature: ");
  Serial.println(myData.temp);

  int segment = 0;
//map temperature 0 - snow flake, 6 - max heat
  if(myData.temp <= 8) segment = 0; // 8 
  else if (myData.temp <= 10) segment = 1;  // 1 - 9,5
  else if (myData.temp <= 13) segment = 2;  // 2 - 13
  else if (myData.temp <= 17) segment = 3;  // 3 - 16,5
  else if (myData.temp <= 20) segment = 4;  // 4 - 20
  else if (myData.temp <= 24) segment = 5;  // 5 - 23,5
  else segment = 6;                         // 6 - 28

  int targetStep = map(segment, 0, 6, 0, stepsPerRevolution);
  Serial.println(segment);
  // Move the stepper to the target position
  stepper.moveTo(2*targetStep);
  Serial.print("Moving to step: ");
  Serial.println(targetStep);
  } else {
  Serial.println("Unknown type");
  }
}

void sendAck(uint8_t *mac, uint16_t msgId) {
  struct_message ackMsg;
  ackMsg.msgType = ACK;
  ackMsg.msgId = msgId;
  ackMsg.temp = 0;

  esp_now_send(mac, (uint8_t *)&ackMsg, sizeof(ackMsg));
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.channel(ESPNOW_CHANNEL); // delete if needed - using for working esp + wifi at the same time

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register to receive the data
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  // Setup the stepper motor
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
}

void loop() {
  stepper.run(); // Always run to move towards target position
}
