#ifndef RADIATOR_MANAGER_H
#define RADIATOR_MANAGER_H

#include "Communications.h"
#include "Messages.h"

#define DEFAULT_TEMP 20
#define MAX_RADIATORS 10

#define MIN_TEMP 8
#define MAX_TEMP 28

typedef struct {
  uint8_t mac[6];
  char name[16];
  uint8_t curr_temp; // hold current temp for each radiator
  bool ackReceived; 
} Radiator;

class RadiatorManager {
public:
  RadiatorManager(Communications& comsRef);

  void processTemperatureResponse(const uint8_t* mac, const TemperatureResponse& response);
  void handleDiscovery(const Peer& peer);

  void sendTemperatureToAll(uint8_t temperature);
  void sendTemperatureTo(int index, uint8_t temperature);
  void sendTemperatureCommand(const uint8_t* mac, uint8_t temperature);

  const Radiator* getRadiators() const;
  int getNumRadiators() const;
  const char* getRadiatorName(int index) const;
  uint8_t getRadiatorTemperature(int index) const;

  bool isAcked(int index) const;
private:
  Radiator radiators[MAX_RADIATORS];
  int numRadiators = 0;
  uint8_t commonTemp = DEFAULT_TEMP;

  Communications& coms;

  int findRadiatorIndex(const uint8_t* mac) const;
};

#endif