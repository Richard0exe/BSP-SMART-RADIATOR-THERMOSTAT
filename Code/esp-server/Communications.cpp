#include "Communications.h"

Communications* Communications::instance = nullptr;

Communications::Communications() {}

const uint8_t Communications::broadcastAddr[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void Communications::begin() {
  if (instance == nullptr) {
    instance = this;
  } 

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  addPeer(broadcastAddr);

  printMac();
}

void Communications::broadcastDiscovery() {
  DiscoveryPayload payload = {};
  strncpy(payload.name, deviceName, MAX_NAME_LEN - 1);

  send(broadcastAddr, DISCOVERY_MSG_TYPE, reinterpret_cast<const uint8_t*>(&payload), sizeof(payload));

  Serial.println("Discovery message broadcasted.");
}

void Communications::printMac() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.print("MAC: ");
  Serial.println(macToString(mac));
}

String Communications::macToString(const uint8_t* mac) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buffer);
}

esp_err_t Communications::send(const uint8_t* addr, uint8_t type, const uint8_t* payload, uint8_t length) {
  if (length > 248) {
    Serial.println("Payload too large for ESP-NOW");
    return ESP_ERR_INVALID_SIZE;
  }

  uint8_t buffer[250]; // 2 bytes for header + up to 248 bytes of payload
  MessageHeader header = { MESSAGE_MAGIC, type, length };

  memcpy(buffer, &header, sizeof(header));
  memcpy(buffer + sizeof(header), payload, length);

  esp_err_t result = esp_now_send(addr, buffer, sizeof(header) + length);
  if (result != ESP_OK) {
    Serial.println("Failed to send message");
    Serial.print(result);
    Serial.print(" - ");
    Serial.println(esp_err_to_name(result));
  }

  return result;
}


void Communications::setReceiveHandler(std::function<void(const uint8_t*, uint8_t, const uint8_t*, int)> handler) {
  userRecvHandler = handler;
}

void Communications::setSendHandler(std::function<void(const uint8_t*, esp_now_send_status_t)> handler) {
  userSendHandler = handler;
}

void Communications::setDiscoveryHandler(std::function<void(const Peer&)> handler) {
  discoveryHandler = handler;
}

void Communications::setName(const char* name) {
  strncpy(deviceName, name, MAX_NAME_LEN - 1);
  deviceName[MAX_NAME_LEN - 1] = '\0';
}

void Communications::addToDiscoveryWhitelist(const char* name) {
  if (whitelistCount >= MAX_WHITELIST) {
    Serial.println("Whitelist full, cannot add more names.");
    return;
  }

  strncpy(whitelist[whitelistCount], name, MAX_NAME_LEN - 1);
  whitelist[whitelistCount][MAX_NAME_LEN - 1] = '\0';
  whitelistCount++;
}

int Communications::getPeerCount() const {
  return peerCount;
}

const Peer* Communications::getPeer(int index) const {
  if (index < 0 || index >= peerCount) return nullptr;
  return &knownPeers[index];
}

const Peer* Communications::getPeerByName(const char* name) const {
  for (int i = 0; i < peerCount; ++i) {
    if (strncmp(knownPeers[i].name, name, MAX_NAME_LEN) == 0) {
      return &knownPeers[i];
    }
  }
  return nullptr;
}

void Communications::onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.printf("Sent to %s %s\n", macToString(mac_addr).c_str(),
                status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

  if (instance && instance->userSendHandler) {
    instance->userSendHandler(mac_addr, status);
  }
}

void Communications::onDataRecv(const esp_now_recv_info_t* recvInfo, const uint8_t* data, int len) {
  if (!instance) return;

  if (len < sizeof(MessageHeader)) {
    Serial.println("Too short for header");
    return;
  }

  const MessageHeader* header = (const MessageHeader*)data;

  if (header->magic != MESSAGE_MAGIC) {
    Serial.println("Invalid message magic");
    return;
  }

  if (len != sizeof(MessageHeader) + header->length) {
    Serial.printf("Payload length mismatch: expected %d, got %d\n", header->length, len - sizeof(MessageHeader));
    return;
  }

  const uint8_t* payloadData = data + sizeof(MessageHeader);

  if (isDiscoveryMessage(*header)) {
    if (header->length != sizeof(DiscoveryPayload)) {
      Serial.println("Invalid discovery payload length");
      return;
    }

    DiscoveryPayload payload;
    memcpy(&payload, payloadData, sizeof(payload));
    instance->handleDiscovery(recvInfo->src_addr, payload);
    return;
  }

  if (instance->userRecvHandler) {
    instance->userRecvHandler(recvInfo->src_addr, header->type, payloadData, header->length);
  }
}

bool Communications::isDiscoveryMessage(const MessageHeader& msg) {
  return msg.type == DISCOVERY_MSG_TYPE;
}

void Communications::handleDiscovery(const uint8_t* mac, const DiscoveryPayload& payload) {
  // Reject if not on whitelist
  bool allowed = false;
  if (whitelistCount == 0) {
    allowed = true;
  } else {
    for (int i = 0; i < whitelistCount; ++i) {
      if (strncmp(payload.name, whitelist[i], MAX_NAME_LEN) == 0) {
        allowed = true;
        break;
      }
    }
  }

  if (!allowed) {
    Serial.printf("Discovery ignored: '%s' not in whitelist.\n", payload.name);
    return;
  }
  
  if (isKnownPeer(mac)) {
    if (!payload.isResponse) {
      sendDiscoveryResponse(mac);
    }
    
    return;
  }

  if (peerCount >= MAX_PEERS) {
    Serial.println("Max peers reached; ignoring new discovery.");
    return;
  }

  if (!addPeer(mac)) return;

  Peer& peer = knownPeers[peerCount++];
  memcpy(peer.mac, mac, 6);
  strncpy(peer.name, payload.name, MAX_NAME_LEN - 1);
  peer.name[MAX_NAME_LEN - 1] = '\0';

  Serial.printf("Discovered new peer: %s (%s)\n", peer.name, macToString(mac).c_str());

  if (!payload.isResponse) {
    sendDiscoveryResponse(mac);
  }

  if (discoveryHandler) {
    discoveryHandler(peer);
  }
}

bool Communications::isKnownPeer(const uint8_t* mac) {
  for (int i = 0; i < peerCount; ++i) {
    if (memcmp(mac, knownPeers[i].mac, 6) == 0) return true;
  }
  return false;
}

bool Communications::addPeer(const uint8_t* mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    return true;
  } else {
    Serial.println("Failed to add peer");
    return false;
  }
}

void Communications::sendDiscoveryResponse(const uint8_t* mac) {
  DiscoveryPayload responsePayload = {};
  strncpy(responsePayload.name, deviceName, MAX_NAME_LEN - 1);
  responsePayload.isResponse = true;

  send(mac, DISCOVERY_MSG_TYPE, reinterpret_cast<const uint8_t*>(&responsePayload), sizeof(responsePayload));

  Serial.printf("Sent discovery response to %s\n", macToString(mac).c_str());
}

