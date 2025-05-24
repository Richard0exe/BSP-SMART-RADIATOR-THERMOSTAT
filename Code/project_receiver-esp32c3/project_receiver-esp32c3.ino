#include <WiFi.h>
#include "esp_wifi.h"
#include <esp_now.h>
#include <AccelStepper.h>

#define DEBUG FALSE // CHANGE TO TRUE TO ENABLE SERIAL OUTPUTS 
#define ESPNOW_CHANNEL 6
#define STEPS_PER_REVOLUTION 2048

#define IN1 5 // D5
#define IN2 4 // D1
#define IN3 6 // D6
#define IN4 7 // D7

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
void OnDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);

  //   // Add peer dynamically
  // esp_now_peer_info_t peerInfo = {};
  // memcpy(peerInfo.peer_addr, recvInfo->src_addr, 6);
  // peerInfo.channel = ESPNOW_CHANNEL;
  // peerInfo.encrypt = false;

  // if (!esp_now_is_peer_exist(recvInfo->src_addr)) {
  //   if (esp_now_add_peer(&peerInfo) == ESP_OK) {
  //     Serial.println("New peer added successfully.");
  //   } else {
  //     Serial.println("Failed to add new peer.");
  //   }
  // }

  if (myData.msgType == DATA) {
    Serial.println("Message type: DATA");
    //sendAck((uint8_t*)recvInfo->src_addr, myData.msgId); // send back ACK
    Serial.print("Received Temperature: ");
    Serial.println(myData.temp);

    int segment = 0;
    if (myData.temp <= 8) segment = 0;
    else if (myData.temp <= 10) segment = 1;
    else if (myData.temp <= 13) segment = 2;
    else if (myData.temp <= 17) segment = 3;
    else if (myData.temp <= 20) segment = 4;
    else if (myData.temp <= 24) segment = 5;
    else segment = 6;

    int targetStep = map(segment, 0, 6, 0, stepsPerRevolution);
    stepper.moveTo(2 * targetStep);
    Serial.print("Moving to step: ");
    Serial.println(targetStep);
  } else {
    Serial.println("Unknown type");
  }
}

// void sendAck(uint8_t *mac, uint16_t msgId) {
//   struct_message ackMsg;
//   ackMsg.msgType = ACK;
//   ackMsg.msgId = msgId;
//   ackMsg.temp = 0;

//   esp_err_t result = esp_now_send(mac, (uint8_t *)&ackMsg, sizeof(ackMsg));
//   if (result == ESP_OK) {
//     Serial.println("ACK sent successfully");
//   } else {
//     Serial.print("Failed to send ACK. Error code: ");
//     Serial.println(result);
//   }
// }

void setup() {
  // Initialize Serial Monitor
  Serial.println("Booting...");
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  //WiFi.channel(ESPNOW_CHANNEL);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register to receive the data
  esp_now_register_recv_cb(OnDataRecv);

  // Setup the stepper motor
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
}

void loop() {
  stepper.run(); // Always run to move towards target position
}
