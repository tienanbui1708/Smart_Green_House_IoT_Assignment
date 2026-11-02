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
};

struct RpcRequest {
  char id[RPC_ID_MAX];
  char payload[RPC_PAYLOAD_MAX];
};

bool linkBegin(const LinkConfig &cfg);   // false neu thieu RAM (queue/buffer)
void linkService();                      // goi moi vong lap task, chu ky <= 100 ms
bool linkWifiUp();
bool linkMqttUp();
bool linkTakeJustConnected();            // true mot lan sau moi lan MQTT ket noi lai
int8_t linkRssi();
bool linkPublish(const char *topic, const char *payload);
bool linkNextRpc(RpcRequest &out);       // khong chan
bool linkRespondRpc(const char *id, const char *reason);  // reason = nullptr -> {"ok":true}
