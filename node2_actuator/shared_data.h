// node2_actuator/shared_data.h
// Du lieu dung chung giua cac task. Chi truy cap qua ham duoi day (co mutex).
#pragma once
#include <Arduino.h>

// Ban sao du lieu Node 1 nhan qua ESP-NOW (hoac du lieu gia khi USE_FAKE_SENSOR_DATA).
struct SensorView {
  float temperature;   // SENTINEL_T khi DHT20 Node 1 loi
  float humidity;      // SENTINEL_H khi DHT20 Node 1 loi
  uint8_t soil;
  uint8_t aiClass;     // AI_CLASS_*
  uint8_t heaterRemote;  // muc suoi hien tai cua Node 1
  uint32_t seq;        // 0 = chua nhan goi nao
  uint32_t receivedMs;
};

struct ActuatorState {
  bool autoMode;
  bool fan;
  bool pump;
  bool dataStale;      // > ESPNOW_STALE_MS khong co du lieu moi
  bool dataFault;      // Node 1 bao cam bien loi (ai_class 3 / sentinel)
};

struct NetState {
  bool wifi;
  bool mqtt;
  int8_t rssi;
};

// Canh bao tu RuleChain (RPC cloudAlert). Chi hien thi, khong doi chap hanh.
struct CloudAlertState {
  uint8_t level;      // 0 = khong co; 1 = canh bao (vang); 2 = nghiem trong (do)
  char msg[17];       // toi da 16 ky tu ASCII + '\0'
  uint32_t expireMs;  // millis() khi het hieu luc
};

enum class CmdResult : uint8_t { Ok, RejectedAutoMode };

void sharedInit();

void sharedSetSensor(const SensorView &view);
SensorView sharedGetSensor();

ActuatorState sharedGetActuators();
void sharedSetAutoMode(bool autoMode);
CmdResult sharedSetManualFan(bool on);
CmdResult sharedSetManualPump(bool on);
// Luon cap nhat co stale/fault; chi doi chap hanh khi dang Auto.
void sharedApplyAuto(bool fan, bool pump, bool dataStale, bool dataFault);

void sharedSetNet(const NetState &net);
NetState sharedGetNet();

// level phai 0..2 (kiem tra o noi goi). level 0 xoa ngay; goi lai thi ghi de va gia han.
void sharedSetAlert(uint8_t level, const char *msg, uint32_t durationMs = 10000);
CloudAlertState sharedGetAlert();  // ban sao; da het han thi tra level 0
