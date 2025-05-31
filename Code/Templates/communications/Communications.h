#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <functional>

#define MAX_PEERS 10
#define MAX_NAME_LEN 32
#define DISCOVERY_MSG_TYPE 0
#define MESSAGE_MAGIC 0x42A7

typedef struct {
  uint16_t magic;
  uint8_t type; // 0 = discovery, user-defined types > 0
  uint8_t length; // length of the payload
} MessageHeader;

struct Peer {
  uint8_t mac[6];
  char name[MAX_NAME_LEN];
};

struct DiscoveryPayload {
  char name[MAX_NAME_LEN];
};

class Communications {
public:
  static const uint8_t broadcastAddr[6];

  Communications();

  void begin();
  void broadcastDiscovery();
  esp_err_t send(const uint8_t* addr, uint8_t type, const uint8_t* payload, uint8_t length);
  template <typename T>
  esp_err_t send(const uint8_t* addr, uint8_t type, const T& payload) {
    static_assert(sizeof(T) <= 248, "Payload too large for ESP-NOW");

    return send(addr, type, reinterpret_cast<const uint8_t*>(&payload), sizeof(T));
  }

  void setReceiveHandler(std::function<void(const uint8_t* mac, uint8_t type, const uint8_t* data, int len)> handler);
  void setSendHandler(std::function<void(const uint8_t*, esp_now_send_status_t)> handler);
  void setName(const char* name);

  int getPeerCount() const;
  const Peer* getPeer(int index) const;

  static String macToString(const uint8_t* mac);
  static void printMac();

private:
  static Communications* instance;

  char deviceName[MAX_NAME_LEN] = "Unknown";

  Peer knownPeers[MAX_PEERS];
  int peerCount = 0;

  static void onDataRecv(const esp_now_recv_info_t* recvInfo, const uint8_t* data, int len);
  static void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status);

  static bool isDiscoveryMessage(const MessageHeader& msg);
  void handleDiscovery(const uint8_t* mac, const DiscoveryPayload& payload);
  bool isKnownPeer(const uint8_t* mac);
  bool addPeer(const uint8_t* mac);

  std::function<void(const uint8_t*, uint8_t, const uint8_t*, int)> userRecvHandler;
  std::function<void(const uint8_t*, esp_now_send_status_t)> userSendHandler;
};

#endif
