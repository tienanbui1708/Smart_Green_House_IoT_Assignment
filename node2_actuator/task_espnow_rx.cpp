// node2_actuator/task_espnow_rx.cpp
// Nhan EspNowSensorPkt tu Node 1, cap nhat shared_data, tra EspNowAckPkt (T06, TASKS_SPEC 3.3).
// Mat du lieu > ESPNOW_STALE_MS: taskControl tu vao safe mode (dua tren receivedMs).
#include <WiFi.h>
#include <esp_now.h>
#include <app_log.h>
#include <protocol.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
struct RxItem {
  EspNowSensorPkt pkt;
  uint8_t src[6];
};

struct RxStats {
  uint32_t rx;      // goi hop le
  uint32_t lost;    // khoang trong seq
  uint32_t dup;     // trung seq
  uint32_t reboot;  // seq giam: Node 1 khoi dong lai
  uint32_t badVer;
  uint32_t badSrc;
  uint32_t ackErr;
};

QueueHandle_t rxQueue = nullptr;
volatile uint32_t cbBadLen = 0;
volatile uint32_t cbQueueFull = 0;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
#else
void onNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  // Chay trong task Wi-Fi (khong phai ngat): chi kiem tra do dai + day queue, khong dung ...FromISR.
  if (len != (int)sizeof(EspNowSensorPkt)) {
    cbBadLen++;
    return;
  }
  RxItem it;
  memcpy(&it.pkt, data, sizeof(it.pkt));
  memcpy(it.src, mac, sizeof(it.src));
  if (xQueueSend(rxQueue, &it, 0) != pdTRUE) cbQueueFull++;
}

bool initEspNow() {
  // Wi-Fi do coreiot_link bat; chi cho, khong doi che do Wi-Fi.
  while (WiFi.getMode() == WIFI_MODE_NULL) vTaskDelay(pdMS_TO_TICKS(ESPNOW_WIFI_WAIT_MS));
  esp_err_t err = esp_now_init();
  if (err != ESP_OK) {
    appLog("NOW", "esp_now_init failed err=%d", (int)err);
    return false;
  }
  esp_now_register_recv_cb(onNowRecv);
  appLog("NOW", "ready, self=%s ch=%d", WiFi.macAddress().c_str(), (int)WiFi.channel());
  return true;
}

bool sourceAllowed(const uint8_t *src) {
#ifdef NODE1_MAC
  static const uint8_t node1Mac[6] = NODE1_MAC;
  return memcmp(src, node1Mac, sizeof(node1Mac)) == 0;
#else
  (void)src;
  return true;
#endif
}

bool ensurePeer(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t err = esp_now_add_peer(&peer);
  if (err != ESP_OK) appLog("NOW", "add peer failed err=%d", (int)err);
  return err == ESP_OK;
}

void sendAck(const RxItem &it, RxStats &st) {
  EspNowAckPkt ack = {(uint8_t)PROTOCOL_VERSION, it.pkt.seq, millis()};
  if (!ensurePeer(it.src) || esp_now_send(it.src, (const uint8_t *)&ack, sizeof(ack)) != ESP_OK) st.ackErr++;
}

// Tra true neu goi mang du lieu moi (khong trung). Chiu duoc Node 1 khoi dong lai (TASKS_SPEC 3.3.3).
bool acceptSeq(uint32_t seq, uint32_t &lastSeq, RxStats &st) {
  if (lastSeq == 0) {
    lastSeq = seq;
    return true;
  }
  if (seq == lastSeq) {
    st.dup++;
    return false;
  }
  if (seq < lastSeq) {
    appLog("NOW", "node1 reboot detected (seq %lu -> %lu)", (unsigned long)lastSeq, (unsigned long)seq);
    st.reboot++;
  } else if (seq > lastSeq + 1) {
    st.lost += seq - lastSeq - 1;
  }
  lastSeq = seq;
  return true;
}

void storeSample(const EspNowSensorPkt &p) {
#if !USE_FAKE_SENSOR_DATA
  SensorView v = {};
  v.temperature = p.temperature;
  v.humidity = p.humidity;
  v.soil = p.soil;
  v.aiClass = p.ai_class;
  v.heaterRemote = p.heater;
  v.seq = p.seq;
  v.receivedMs = millis();
  sharedSetSensor(v);
#else
  (void)p;
#endif
}

void handlePacket(const RxItem &it, uint32_t &lastSeq, RxStats &st) {
  const EspNowSensorPkt &p = it.pkt;
  if (p.ver != PROTOCOL_VERSION) {
    st.badVer++;
    appLog("NOW", "drop seq=%lu: ver=%u != %u", (unsigned long)p.seq, p.ver, PROTOCOL_VERSION);
    return;
  }
  if (!sourceAllowed(it.src)) {
    st.badSrc++;
    return;
  }
  if (acceptSeq(p.seq, lastSeq, st)) {
    storeSample(p);
    st.rx++;
#if DEBUG_LOG
    appLog("NOW", "rx seq=%lu T=%.1f H=%.1f soil=%u cls=%u ht=%u", (unsigned long)p.seq, p.temperature,
           p.humidity, p.soil, p.ai_class, p.heater);
#endif
  }
  sendAck(it, st);  // ca goi trung cung Ack: Ack truoc co the da mat
}

void logStats(const RxStats &st) {
  uint32_t expected = st.rx + st.lost;
  uint32_t lossPct = expected ? (uint32_t)((uint64_t)st.lost * 100 / expected) : 0;
  appLog("NOW", "stats rx=%lu lost=%lu loss=%lu%% dup=%lu reboot=%lu bad_ver=%lu bad_src=%lu bad_len=%lu qfull=%lu ack_err=%lu",
         (unsigned long)st.rx, (unsigned long)st.lost, (unsigned long)lossPct, (unsigned long)st.dup,
         (unsigned long)st.reboot, (unsigned long)st.badVer, (unsigned long)st.badSrc, (unsigned long)cbBadLen,
         (unsigned long)cbQueueFull, (unsigned long)st.ackErr);
}
}  // namespace

void taskEspnowRx(void *) {
  rxQueue = xQueueCreate(ESPNOW_RX_QUEUE_LEN, sizeof(RxItem));
  if (rxQueue == nullptr || !initEspNow()) {
    appLog("NOW", "init FAILED, task stopped (Node 2 stays in safe mode)");
    vTaskDelete(nullptr);
  }
  uint32_t lastSeq = 0;
  RxStats st = {};
  uint32_t nextStatsMs = millis() + STATS_PERIOD_MS;
  for (;;) {
    RxItem it;
    // Timeout de van in thong ke khi khong co goi (Node 1 tat).
    if (xQueueReceive(rxQueue, &it, pdMS_TO_TICKS(ESPNOW_RX_WAIT_MS)) == pdTRUE) handlePacket(it, lastSeq, st);
    if ((int32_t)(millis() - nextStatsMs) >= 0) {
      logStats(st);
      nextStatsMs += STATS_PERIOD_MS;
    }
  }
}
