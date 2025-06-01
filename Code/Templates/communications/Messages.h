// Messages.h
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

// Message types (start from 1; 0 is reserved for discovery)
enum MessageType : uint8_t {
  MSG_TYPE_TEMPERATURE = 1
  // Add more as needed
};

// Structure to receive data (temperature)
struct TemperaturePayload {
  uint8_t temperature;  // Temperature value received
};

#endif // MESSAGES_H