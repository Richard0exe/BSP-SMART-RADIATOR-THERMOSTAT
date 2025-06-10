// Messages.h
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

// Message types (start from 1; 0 is reserved for discovery)
enum MessageType : uint8_t {
  MSG_TYPE_TEMPERATURE_COMMAND = 1,
  MSG_TYPE_TEMPERATURE_RESPONSE = 2
  // Add more as needed
};

// Structure to receive data (temperature)
struct TemperatureCommand {
  uint8_t temperature;  // Temperature value received
};

struct TemperatureResponse {
  uint8_t temperature;  // Temperature value set
  bool success;
};

#endif // MESSAGES_H