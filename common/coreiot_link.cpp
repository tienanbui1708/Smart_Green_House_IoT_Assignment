// common/coreiot_link.cpp
#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include "coreiot_link.h"
#include "protocol.h"
#include "app_log.h"

namespace {
enum class WifiPhase : uint8_t { Idle, Connecting, Up };

LinkConfig cfg = {};
WiFiClient netClient;
PubSubClient mqtt(netClient);
QueueHandle_t rpcQueue = nullptr;

WifiPhase wifiPhase = WifiPhase::Idle;
uint8_t wifiFailures = 0;
uint32_t wifiPhaseMs = 0;     // luc bat dau Connecting / luc duoc thu lai khi Idle
uint32_t mqttNextTryMs = 0;
bool mqttWasUp = false;
bool justConnected = false;
volatile uint32_t rpcDropped = 0;

RescueApConfig ap = {};
bool apActive = false;
bool everConnected = false;
uint32_t staDownSinceMs = 0;  // luc khoi dong hoac luc mat STA
uint32_t lastApClientMs = 0;  // lan cuoi thay co may trong AP
uint8_t lastStaChannel = 0;   // kenh router lan ket noi gan nhat

bool timeReached(uint32_t target) {
  return (int32_t)(millis() - target) >= 0;
}

void maskToken(char *out, size_t size) {
  snprintf(out, size, "%.4s****", cfg.token);
}

// Chay ben trong mqtt.loop(). Chi copy + day vao queue: bo dem PubSubClient bi ghi de khi publish.
void onMqttMessage(char *topic, uint8_t *payload, unsigned int len) {
  const size_t prefixLen = strlen(TOPIC_RPC_REQUEST_PREFIX);
  if (strncmp(topic, TOPIC_RPC_REQUEST_PREFIX, prefixLen) != 0) return;
  const char *id = topic + prefixLen;
  RpcRequest req;
  size_t idLen = strlen(id);
  if (idLen == 0 || idLen >= sizeof(req.id) || len >= sizeof(req.payload)) {
    rpcDropped++;
    return;
  }
  memcpy(req.id, id, idLen + 1);
  memcpy(req.payload, payload, len);
  req.payload[len] = '\0';
  if (xQueueSend(rpcQueue, &req, 0) != pdTRUE) rpcDropped++;
}

// Kenh ESP-NOW: co dinh neu cau hinh, khong thi theo kenh router lan cuoi.
uint8_t espnowChannel() {
  return cfg.channel != 0 ? cfg.channel : lastStaChannel;
}

// Sau disconnect radio co the dung o kenh quet cuoi; dua ve kenh ESP-NOW de hai node van nghe nhau.
void holdChannel() {
  uint8_t ch = espnowChannel();
  if (ch == 0) return;
  esp_err_t err = esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  if (err != ESP_OK) appLog("WIFI", "set channel %u failed err=%d", ch, (int)err);
}

// Dang co nguoi dung trong AP: tam dung quet STA de AP khong bi nhay kenh (TASKS_SPEC 4.4.2).
bool staAttemptAllowed() {
  if (!apActive) return true;
  if (WiFi.softAPgetStationNum() > 0) {
    lastApClientMs = millis();
    return false;
  }
  return millis() - lastApClientMs >= ap.idleRetryMs;
}

void startAp() {
  uint8_t ch = espnowChannel() != 0 ? espnowChannel() : 1;  // cung kenh ESP-NOW
  WiFi.mode(WIFI_AP_STA);
  bool ok = WiFi.softAP(ap.ssid, ap.pass, ch);
  apActive = ok;
  lastApClientMs = millis();
  appLog("WIFI", "rescue AP '%s' %s ch=%u ip=%s", ap.ssid, ok ? "up" : "FAILED", ch,
         WiFi.softAPIP().toString().c_str());
}

void stopAp() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
  appLog("WIFI", "rescue AP off");
}

void serviceAp() {
  if (ap.ssid == nullptr) return;
  if (wifiPhase == WifiPhase::Up) {
    if (apActive) stopAp();
    return;
  }
  uint32_t limit = everConnected ? ap.lostTimeoutMs : ap.bootTimeoutMs;
  if (!apActive && millis() - staDownSinceMs >= limit) startAp();
}

void startWifiAttempt() {
  appLog("WIFI", "connecting to '%s' (attempt %u)", cfg.ssid, wifiFailures + 1);
  WiFi.disconnect();
  // Kenh co dinh: chi quet kenh do, ESP-NOW khong bi keo sang kenh khac khi router doi/tat.
  WiFi.begin(cfg.ssid, cfg.pass, cfg.channel);
  wifiPhase = WifiPhase::Connecting;
  wifiPhaseMs = millis();
}

void scheduleWifiRetry() {
  size_t idx = wifiFailures < cfg.wifiBackoffCount ? wifiFailures : cfg.wifiBackoffCount - 1;
  uint32_t wait = cfg.wifiBackoffMs[idx];
  if (wifiFailures < UINT8_MAX) wifiFailures++;
  appLog("WIFI", "not connected (status=%d), retry in %lu ms", WiFi.status(), (unsigned long)wait);
  WiFi.disconnect();
  holdChannel();
  wifiPhase = WifiPhase::Idle;
  wifiPhaseMs = millis() + wait;
}

void serviceWifi() {
  bool up = WiFi.status() == WL_CONNECTED;
  switch (wifiPhase) {
    case WifiPhase::Up:
      if (!up) {
        appLog("WIFI", "connection lost");
        holdChannel();
        wifiPhase = WifiPhase::Idle;
        wifiPhaseMs = millis();
        staDownSinceMs = millis();
      }
      break;
    case WifiPhase::Connecting:
      if (up) {
        appLog("WIFI", "connected ip=%s rssi=%d ch=%d", WiFi.localIP().toString().c_str(), (int)WiFi.RSSI(), (int)WiFi.channel());
        wifiPhase = WifiPhase::Up;
        wifiFailures = 0;
        everConnected = true;
        lastStaChannel = (uint8_t)WiFi.channel();
      } else if (!staAttemptAllowed()) {
        WiFi.disconnect();  // co nguoi vao AP giua luc quet: dung quet ngay
        holdChannel();
        wifiPhase = WifiPhase::Idle;
        wifiPhaseMs = millis();
      } else if (millis() - wifiPhaseMs >= cfg.wifiAttemptTimeoutMs) {
        scheduleWifiRetry();
      }
      break;
    case WifiPhase::Idle:
      if (timeReached(wifiPhaseMs) && staAttemptAllowed()) startWifiAttempt();
      break;
  }
}

void connectMqtt() {
  char masked[12];
  maskToken(masked, sizeof(masked));
  appLog("MQTT", "connecting %s:%u token=%s", cfg.host, cfg.port, masked);
  if (!mqtt.connect(cfg.clientId, cfg.token, nullptr)) {
    // Bang ma loi: CLAUDE.md 6.3 (4/5 = token sai)
    appLog("MQTT", "connect failed state=%d, retry in %lu ms", mqtt.state(), (unsigned long)cfg.mqttRetryMs);
    mqttNextTryMs = millis() + cfg.mqttRetryMs;
    return;
  }
  bool subOk = mqtt.subscribe(TOPIC_RPC_REQUEST_SUB);
  appLog("MQTT", "connected, subscribe RPC %s", subOk ? "ok" : "FAILED");
  mqttWasUp = true;
  justConnected = true;
}

void serviceMqtt() {
  if (wifiPhase != WifiPhase::Up) {
    if (mqtt.connected()) mqtt.disconnect();
    mqttWasUp = false;
    return;
  }
  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }
  if (mqttWasUp) {
    appLog("MQTT", "disconnected state=%d", mqtt.state());
    mqttWasUp = false;
    mqttNextTryMs = millis();
  }
  if (timeReached(mqttNextTryMs)) connectMqtt();
}
}  // namespace

void linkEnableRescueAp(const RescueApConfig &a) {
  ap = a;
}

bool linkBegin(const LinkConfig &c) {
  cfg = c;
  rpcQueue = xQueueCreate(RPC_QUEUE_LEN, sizeof(RpcRequest));
  if (rpcQueue == nullptr) return false;

  WiFi.persistent(false);       // khong ghi flash moi lan begin
  WiFi.mode(WIFI_STA);          // ESP-NOW (M5) cung can STA
  WiFi.setAutoReconnect(false); // tu quan ly backoff
  WiFi.setSleep(false);         // modem-sleep lam ESP-NOW rot goi khi STA dang ket noi
  holdChannel();
  mqtt.setServer(cfg.host, cfg.port);
  mqtt.setKeepAlive(cfg.keepAliveS);
  mqtt.setSocketTimeout(cfg.socketTimeoutS);
  mqtt.setCallback(onMqttMessage);
  if (!mqtt.setBufferSize(cfg.bufferSize)) return false;
  wifiPhase = WifiPhase::Idle;
  wifiPhaseMs = millis();
  staDownSinceMs = millis();
  return true;
}

void linkService() {
  serviceWifi();
  serviceAp();
  serviceMqtt();
  if (rpcDropped > 0) {
    appLog("MQTT", "dropped %lu RPC (too long or queue full)", (unsigned long)rpcDropped);
    rpcDropped = 0;
  }
}

bool linkWifiUp() {
  return wifiPhase == WifiPhase::Up;
}

bool linkApActive() {
  return apActive;
}

uint8_t linkApClients() {
  return apActive ? (uint8_t)WiFi.softAPgetStationNum() : 0;
}

uint8_t linkChannel() {
  return (uint8_t)WiFi.channel();
}

bool linkMqttUp() {
  return mqtt.connected();
}

bool linkTakeJustConnected() {
  bool v = justConnected;
  justConnected = false;
  return v;
}

int8_t linkRssi() {
  return linkWifiUp() ? (int8_t)WiFi.RSSI() : 0;
}

bool linkPublish(const char *topic, const char *payload) {
  if (!mqtt.connected()) return false;
  return mqtt.publish(topic, payload);  // PubSubClient chi publish QoS 0
}

bool linkNextRpc(RpcRequest &out) {
  return rpcQueue != nullptr && xQueueReceive(rpcQueue, &out, 0) == pdTRUE;
}

bool linkRespondRpc(const char *id, const char *reason) {
  char topic[64];
  char body[64];
  snprintf(topic, sizeof(topic), "%s%s", TOPIC_RPC_RESPONSE_PREFIX, id);
  if (reason == nullptr) {
    snprintf(body, sizeof(body), "{\"ok\":true}");
  } else {
    snprintf(body, sizeof(body), "{\"ok\":false,\"reason\":\"%s\"}", reason);
  }
  return linkPublish(topic, body);
}
