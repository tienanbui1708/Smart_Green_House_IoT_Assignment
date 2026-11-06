// node1_sensor/shared_data.h
// Du lieu dung chung giua cac task. Chi truy cap qua ham duoi day (co mutex).
#pragma once
#include <Arduino.h>

struct SensorSample {
  float temperature;  // SENTINEL_T khi DHT20 loi
  float humidity;     // SENTINEL_H khi DHT20 loi
  uint8_t soil;       // % da hieu chinh
  uint16_t soilRaw;
  bool dhtOk;
  uint32_t seq;       // 0 = chua co mau nao
  uint32_t takenMs;
};

struct ControlState {
  bool autoMode;
  uint8_t heater;     // 0..255 suoi mo phong
};

struct NetState {
  bool wifi;
  bool mqtt;
  int8_t rssi;
};

// Thong ke ESP-NOW (task espnow ghi, web/telemetry doc).
struct EspNowStats {
  bool ready;          // esp_now_init + add peer OK
  uint32_t sent;       // so goi esp_now_send tra ESP_OK
  uint32_t acked;      // so Ack hop le (khop seq)
  uint32_t sendErr;    // esp_now_send loi hoac callback bao FAIL
  uint32_t lastRttUs;
  uint32_t lastAckMs;  // millis() luc nhan Ack gan nhat, 0 = chua co
};

enum class CmdResult : uint8_t { Ok, RejectedAutoMode };

void sharedInit();  // goi trong setup() truoc khi tao task

void sharedSetSample(const SensorSample &sample);  // dong thoi bao 'co mau moi' cho task TinyML
SensorSample sharedGetSample();
bool sharedWaitNewSample(uint32_t timeoutMs);      // true khi co mau moi (mot nguoi doi duy nhat)

ControlState sharedGetControl();
void sharedSetAutoMode(bool autoMode);
CmdResult sharedSetManualHeater(uint8_t value);
void sharedSetAutoHeater(uint8_t value);  // bo qua neu dang Manual
bool sharedForceSafe();                   // ep Auto + tat suoi; tra true neu trang thai doi
void sharedHoldSafe();                    // tat suoi va khoa den khi khoi dong lai (truoc OTA/restart)

void sharedSetNet(const NetState &net);
NetState sharedGetNet();

// Nhan TinyML da lam muot (AI_CLASS_*); AI_CLASS_BOOTING khi chua du cua so.
void sharedSetAiClass(uint8_t cls);
uint8_t sharedGetAiClass();

void sharedSetEspNow(const EspNowStats &stats);
EspNowStats sharedGetEspNow();

// Bus I2C dung chung cho DHT20 (task sensor) va LCD (task display).
bool i2cLock(uint32_t timeoutMs);
void i2cUnlock();
