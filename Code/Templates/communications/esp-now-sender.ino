#include "Communications.h"

Communications coms;

struct MyPayload {
  int value;
  float reading;
};

void setup() {
  Serial.begin(115200);
  coms.begin();
  coms.setName("radiator");

  coms.setReceiveHandler([](const uint8_t* mac, uint8_t type, const uint8_t* data, int len) {
    if (type == 1 && len == sizeof(MyPayload)) {
      MyPayload payload;
      memcpy(&payload, data, sizeof(payload));
      Serial.printf("Received: %d / %.2f\n", payload.value, payload.reading);
    }
  });

  coms.setSendHandler([](const uint8_t* mac, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK!" : "Send Failed.");
  });

  coms.broadcastDiscovery();
}

void loop() {
  for (int i = 0; i < coms.getPeerCount(); ++i) {
    const Peer* p = coms.getPeer(i);
    Serial.printf("Peer %d: %s [%s]\n", i, p->name, Communications::macToString(p->mac).c_str());
    MyPayload payload = { 42, 3.14f };
    coms.send(p->mac, 1, payload); // type 1 = MyPayload
  }
  delay(10000);
}
