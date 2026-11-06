// common/coreiot_link.h
// Wi-Fi (backoff) + MQTT toi CoreIOT dung chung hai node. Chi goi tu MOT task (task_coreiot).
#pragma once
#include <Arduino.h>

constexpr size_t RPC_ID_MAX = 16;
constexpr size_t RPC_PAYLOAD_MAX = 256;
constexpr size_t RPC_QUEUE_LEN = 4;

struct LinkConfig {
  const char *ssid;
  const char *pass;
  const char *host;
  uint16_t port;
  const char *token;
  const char *clientId;
  uint16_t bufferSize;
  uint16_t keepAliveS;
  uint16_t socketTimeoutS;
  uint32_t mqttRetryMs;
  uint32_t wifiAttemptTimeoutMs;
  const uint32_t *wifiBackoffMs;
  size_t wifiBackoffCount;
  uint8_t channel;  // 0 = theo router; khac 0 = ESPNOW_CHANNEL co dinh (TASKS_SPEC 3.5)
};

// AP cuu ho (TASKS_SPEC 4.4): coreiot_link la chu duy nhat doi che do Wi-Fi.
struct RescueApConfig {
  const char *ssid;
  const char *pass;          // >= 8 ky tu (WPA2)
  uint32_t bootTimeoutMs;    // chua tung ket noi STA sau thoi gian nay -> bat AP
  uint32_t lostTimeoutMs;    // mat STA qua thoi gian nay -> bat AP
  uint32_t idleRetryMs;      // AP khong co may nao trong thoi gian nay -> cho thu lai STA
};

struct RpcRequest {
  char id[RPC_ID_MAX];
  char payload[RPC_PAYLOAD_MAX];
};

void linkEnableRescueAp(const RescueApConfig &ap);  // goi truoc linkBegin; khong goi = khong co AP
// Topic khong phai RPC (OTA): subscribe them moi lan ket noi; handler chay trong mqtt.loop() cua task coreiot,
// payload chi hop le trong luc goi (bo dem PubSubClient bi ghi de khi publish).
typedef bool (*LinkMessageHandler)(const char *topic, const uint8_t *payload, unsigned int len);
void linkSetMessageHandler(LinkMessageHandler handler, const char *const *topics, size_t topicCount);
bool linkBegin(const LinkConfig &cfg);   // false neu thieu RAM (queue/buffer)
void linkService();                      // goi moi vong lap task, chu ky <= 100 ms
bool linkWifiUp();
bool linkApActive();
uint8_t linkApClients();
uint8_t linkChannel();                   // kenh radio hien tai
bool linkMqttUp();
bool linkTakeJustConnected();            // true mot lan sau moi lan MQTT ket noi lai
int8_t linkRssi();
bool linkPublish(const char *topic, const char *payload);
bool linkNextRpc(RpcRequest &out);       // khong chan
bool linkRespondRpc(const char *id, const char *reason);  // reason = nullptr -> {"ok":true}
