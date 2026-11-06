// node1_sensor/task_espnow.cpp
// Gui EspNowSensorPkt cho Node 2 moi ESPNOW_SEND_PERIOD_MS, nhan Ack de do RTT (T06, TASKS_SPEC 3.2).
// Do tre: chi dung RTT tren dong ho Node 1 (RTT/2 la uoc luong mot chieu); rx_ms cua Node 2 khong dung.
#include <WiFi.h>
#include <esp_now.h>
#include <app_log.h>
#include <protocol.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
struct AckItem {
  EspNowAckPkt ack;
  uint32_t rxUs;  // micros() lay ngay trong callback cho RTT chinh xac
};

struct TxSlot {
  uint32_t seq;
  uint32_t txUs;
  bool acked;
};

struct WindowStats {  // reset moi STATS_PERIOD_MS
  uint32_t acked;
  uint64_t rttSumUs;
  uint32_t rttMaxUs;
};

const uint8_t node2Mac[6] = NODE2_MAC;
QueueHandle_t ackQueue = nullptr;
volatile uint32_t cbSendFail = 0;  // callback chi tang bien dem
volatile uint32_t cbBadLen = 0;
TxSlot slots[ESPNOW_RTT_SLOTS] = {};

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onNowRecv(const esp_now_recv_info_t *, const uint8_t *data, int len) {
#else
void onNowRecv(const uint8_t *, const uint8_t *data, int len) {
#endif
  // Chay trong task Wi-Fi (khong phai ngat): chi kiem tra toi thieu + day queue.
  if (len != (int)sizeof(EspNowAckPkt)) {
    cbBadLen++;
    return;
  }
  AckItem it;
  it.rxUs = micros();
  memcpy(&it.ack, data, sizeof(it.ack));
  xQueueSend(ackQueue, &it, 0);
}

void onNowSent(const uint8_t *, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) cbSendFail++;
}

// Wi-Fi do coreiot_link bat (WiFi.mode); task nay chi cho, khong tu doi che do Wi-Fi.
bool initEspNow() {
  while (WiFi.getMode() == WIFI_MODE_NULL) vTaskDelay(pdMS_TO_TICKS(ESPNOW_WIFI_WAIT_MS));
  esp_err_t err = esp_now_init();
  if (err != ESP_OK) {
    appLog("NOW", "esp_now_init failed err=%d", (int)err);
    return false;
  }
  esp_now_register_recv_cb(onNowRecv);
  esp_now_register_send_cb(onNowSent);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, node2Mac, sizeof(node2Mac));
  peer.channel = 0;  // 0 = kenh hien tai cua STA
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  err = esp_now_add_peer(&peer);
  if (err != ESP_OK) {
    appLog("NOW", "add peer failed err=%d", (int)err);
    return false;
  }
  appLog("NOW", "ready, self=%s peer=%02X:%02X:%02X:%02X:%02X:%02X ch=%d", WiFi.macAddress().c_str(),
         node2Mac[0], node2Mac[1], node2Mac[2], node2Mac[3], node2Mac[4], node2Mac[5], (int)WiFi.channel());
  return true;
}

EspNowSensorPkt buildPacket(uint32_t seq) {
  SensorSample s = sharedGetSample();
  ControlState c = sharedGetControl();
  EspNowSensorPkt p = {};
  p.ver = PROTOCOL_VERSION;
  p.seq = seq;
  p.temperature = s.temperature;  // task sensor da dat SENTINEL khi DHT20 loi
  p.humidity = s.humidity;
  p.soil = s.soil;
  p.ai_class = sharedGetAiClass();
  p.heater = c.heater;
  p.tx_ms = millis();
  return p;
}

void sendPacket(uint32_t seq, EspNowStats &st, uint32_t &sendCallErr) {
  EspNowSensorPkt p = buildPacket(seq);
  TxSlot &slot = slots[seq % ESPNOW_RTT_SLOTS];
  slot = {seq, micros(), false};
  esp_err_t err = esp_now_send(node2Mac, (const uint8_t *)&p, sizeof(p));
  if (err != ESP_OK) {
    sendCallErr++;
    appLog("NOW", "send seq=%lu failed err=%d", (unsigned long)seq, (int)err);
  } else {
    st.sent++;
  }
  // callback chi tang cbSendFail (don dieu), task chi doc: khong can khoa.
  st.sendErr = sendCallErr + cbSendFail;
}

void handleAck(const AckItem &it, EspNowStats &st, WindowStats &w, uint32_t &ackBad) {
  TxSlot &slot = slots[it.ack.seq % ESPNOW_RTT_SLOTS];
  if (it.ack.ver != PROTOCOL_VERSION || slot.seq != it.ack.seq || slot.acked) {
    ackBad++;  // sai phien ban, qua cu (slot da bi ghi de) hoac trung
    return;
  }
  slot.acked = true;
  uint32_t rtt = it.rxUs - slot.txUs;
  st.acked++;
  st.lastRttUs = rtt;
  st.lastAckMs = millis();
  w.acked++;
  w.rttSumUs += rtt;
  if (rtt > w.rttMaxUs) w.rttMaxUs = rtt;
  appLog("NOW", "RTT seq=%lu rtt_us=%lu", (unsigned long)it.ack.seq, (unsigned long)rtt);
}

void logStats(const EspNowStats &st, WindowStats &w, uint32_t ackBad) {
  uint32_t pct = st.sent ? (uint32_t)((uint64_t)st.acked * 100 / st.sent) : 0;
  uint32_t avg = w.acked ? (uint32_t)(w.rttSumUs / w.acked) : 0;
  appLog("NOW", "stats sent=%lu acked=%lu ack_rate=%lu%% rtt_avg_us=%lu rtt_max_us=%lu send_err=%lu ack_bad=%lu bad_len=%lu",
         (unsigned long)st.sent, (unsigned long)st.acked, (unsigned long)pct, (unsigned long)avg,
         (unsigned long)w.rttMaxUs, (unsigned long)st.sendErr, (unsigned long)ackBad, (unsigned long)cbBadLen);
  w = {};
}
}  // namespace

void taskEspnow(void *) {
  ackQueue = xQueueCreate(ESPNOW_ACK_QUEUE_LEN, sizeof(AckItem));
  EspNowStats st = {};
  st.ready = ackQueue != nullptr && initEspNow();
  sharedSetEspNow(st);
  if (!st.ready) {
    appLog("NOW", "init FAILED, task stopped");
    vTaskDelete(nullptr);
  }

  uint32_t seq = 0;
  uint32_t ackBad = 0;
  uint32_t sendCallErr = 0;
  WindowStats w = {};
  uint32_t nextSendMs = millis();
  uint32_t nextStatsMs = millis() + STATS_PERIOD_MS;
  for (;;) {
    if ((int32_t)(millis() - nextSendMs) >= 0) {
      sendPacket(++seq, st, sendCallErr);
      nextSendMs += ESPNOW_SEND_PERIOD_MS;  // cong don de chu ky khong troi
    }
    AckItem it;
    if (xQueueReceive(ackQueue, &it, pdMS_TO_TICKS(ESPNOW_WAIT_ACK_MS)) == pdTRUE) handleAck(it, st, w, ackBad);
    if ((int32_t)(millis() - nextStatsMs) >= 0) {
      logStats(st, w, ackBad);
      nextStatsMs += STATS_PERIOD_MS;
    }
    sharedSetEspNow(st);
  }
}
