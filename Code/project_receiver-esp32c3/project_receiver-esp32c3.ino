#include <AccelStepper.h>
#include "Communications.h"
#include "Messages.h"

#define DEBUG FALSE // CHANGE TO TRUE TO ENABLE SERIAL OUTPUTS 
#define ESPNOW_CHANNEL 6
#define STEPS_PER_REVOLUTION 2048

#define IN1 5 // D5
#define IN2 4 // D1
#define IN3 6 // D6
#define IN4 7 // D7

const int stepsPerRevolution = STEPS_PER_REVOLUTION; // For 28BYJ-48 motor

AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

Communications coms;

// Callback function that wilal be executed when data is received
void OnDataRecv(const uint8_t* mac, uint8_t type, const uint8_t* data, int len) {
  switch (type) {
    case MSG_TYPE_TEMPERATURE_COMMAND:
      if (len == sizeof(TemperatureCommand)) {
        TemperatureCommand payload;
        memcpy(&payload, data, sizeof(payload));
        ProcessTemperatureCommand(mac, payload);
      }
      break;

    default:
      Serial.println("Unknown message type");
      break;
  }
}

void ProcessTemperatureCommand(const uint8_t* mac, const TemperatureCommand& payload) {
  Serial.printf("Received Temperature from %s: %d\n", Communications::macToString(mac).c_str(), payload.temperature);

  if (!isServerMac(mac)) {
    Serial.printf("Unauthorized MAC %s tried to change temperature!\n", Communications::macToString(mac).c_str());
    return;
  }

  int segment = 0;
  if (payload.temperature <= 8) segment = 0;
  else if (payload.temperature <= 10) segment = 1;
  else if (payload.temperature <= 13) segment = 2;
  else if (payload.temperature <= 17) segment = 3;
  else if (payload.temperature <= 20) segment = 4;
  else if (payload.temperature <= 24) segment = 5;
  else segment = 6;

  int targetStep = map(segment, 0, 6, 0, stepsPerRevolution);
  stepper.moveTo(2 * targetStep);
  Serial.print("Moving to step: ");
  Serial.println(targetStep);

  // send ack
  sendAckTemperatureResponse(mac, payload, true);
}

bool isServerMac(const uint8_t mac[6]) {
  const Peer* server = coms.getPeerByName("server");
  if (!server) return false;
  return memcmp(mac, server->mac, 6) == 0;
}

void sendAckTemperatureResponse(const uint8_t* mac, const TemperatureCommand& payload, bool success) {
  TemperatureResponse response = {};
  response.temperature = payload.temperature;
  response.success = success;

  esp_err_t result = coms.send(mac, MSG_TYPE_TEMPERATURE_RESPONSE, response);
  if (result == ESP_OK) {
    Serial.println("ACK sent successfully");
  } else {
    Serial.print("Failed to send ACK.");
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

  coms.begin();
  coms.setName("radiator");
  
  // Register to receive the data
  coms.setReceiveHandler(OnDataRecv);

  coms.broadcastDiscovery();

  // Setup the stepper motor
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
}

void loop() {
  stepper.run(); // Always run to move towards target position
}
