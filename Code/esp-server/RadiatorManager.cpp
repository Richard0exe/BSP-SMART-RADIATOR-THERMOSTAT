#include "RadiatorManager.h"

RadiatorManager::RadiatorManager(Communications& comsRef)
  : coms(comsRef) {}

void RadiatorManager::processTemperatureResponse(const uint8_t* mac, const TemperatureResponse& response) {
  int idx = findRadiatorIndex(mac);
  if (idx == -1) {
    Serial.printf("ACK from unknown device [%s]: %d°C\n",
                  Communications::macToString(mac).c_str(), response.temperature);
    return;
  }

  if (!response.success) {
    Serial.printf("Failed to set temp on [%s] (wanted %d°C)\n",
                  Communications::macToString(mac).c_str(), response.temperature);
    return;
  }

  radiators[idx].ackReceived = true;
  Serial.printf("ACK received from %s: Temperature set to %d°C\n", radiators[idx].name, response.temperature);
}

void RadiatorManager::handleDiscovery(const Peer& peer) {
  if (numRadiators >= MAX_RADIATORS) {
    Serial.println("Maximum number of radiators reached. Skipping.");
    return;
  }

  // Check for duplicate MAC
  for (int i = 0; i < numRadiators; ++i) {
    if (memcmp(radiators[i].mac, peer.mac, 6) == 0) {
      return;  // Already added
    }
  }

  // Add new radiator
  Radiator& r = radiators[numRadiators++];
  memcpy(r.mac, peer.mac, 6);

  // Auto-generate name: "Room X"
  snprintf(r.name, sizeof(r.name), "Room %d", numRadiators);

  r.curr_temp = DEFAULT_TEMP;
  r.ackReceived = false;

  Serial.printf("New radiator added: %s [%s]\n", r.name, Communications::macToString(r.mac).c_str());
}

void RadiatorManager::sendTemperatureToAll(uint8_t temperature) {
  for (int i = 0; i < numRadiators; i++) {
    sendTemperatureTo(i, temperature);
  }
}

void RadiatorManager::sendTemperatureTo(int index, uint8_t temperature) {
  if (index < 0 || index >= numRadiators) return;

  if (radiators[index].curr_temp == temperature && radiators[index].ackReceived) return;  // already set

  radiators[index].ackReceived = false;
  radiators[index].curr_temp = temperature;
  sendTemperatureCommand(radiators[index].mac, temperature);
}

void RadiatorManager::sendTemperatureCommand(const uint8_t* mac, uint8_t temperature) {
  TemperatureCommand cmd = {};
  cmd.temperature = temperature;

  esp_err_t result = coms.send(mac, MSG_TYPE_TEMPERATURE_COMMAND, cmd);

  if (result == ESP_OK) {
    Serial.printf("Sent temperature command to [%s]: %d°C\n",
                  Communications::macToString(mac).c_str(), temperature);
  } else {
    Serial.printf("Failed to send temperature command to [%s]: error code %d\n",
                  Communications::macToString(mac).c_str(), result);
  }
}

const Radiator* RadiatorManager::getRadiators() const {
  return radiators;
}

int RadiatorManager::getNumRadiators() const {
  return numRadiators;
}

const char* RadiatorManager::getRadiatorName(int index) const {
  if (index < 0 || index >= numRadiators) {
    return "Invalid";
  }
  return radiators[index].name;
}

uint8_t RadiatorManager::getRadiatorTemperature(int index) const {
  if (index < 0 || index >= numRadiators) {
    return 0;
  }
  return radiators[index].curr_temp;
}


bool RadiatorManager::isAcked(int index) const {
  if (index < 0 || index >= numRadiators) return false;
  return radiators[index].ackReceived;
}

bool RadiatorManager::isAllAcked() const {
  for (int i = 0; i < numRadiators; i++) {
    if (!radiators[i].ackReceived) return false;
  }

  return true;
}

int RadiatorManager::findRadiatorIndex(const uint8_t* mac) const {
  for (int i = 0; i < numRadiators; i++) {
    if (memcmp(mac, radiators[i].mac, 6) == 0) {
      return i;
    }
  }
  return -1;
}