// node2_actuator/shared_data.h
// Du lieu dung chung giua cac task. Chi truy cap qua ham duoi day (co mutex).
#pragma once
#include <Arduino.h>

// Ban sao du lieu Node 1 (M5: tu ESP-NOW; hien tai: du lieu gia).
struct SensorView {
  float temperature;
  float humidity;
  uint8_t soil;
  uint32_t seq;        // 0 = chua nhan goi nao
  uint32_t receivedMs;
};

struct ActuatorState {
  bool autoMode;
  bool fan;
  bool pump;
  bool dataStale;      // > DATA_STALE_MS khong co du lieu moi
};

struct NetState {
  bool wifi;
  bool mqtt;
  int8_t rssi;
};

enum class CmdResult : uint8_t { Ok, RejectedAutoMode };

void sharedInit();

void sharedSetSensor(const SensorView &view);
SensorView sharedGetSensor();

ActuatorState sharedGetActuators();
void sharedSetAutoMode(bool autoMode);
CmdResult sharedSetManualFan(bool on);
CmdResult sharedSetManualPump(bool on);
void sharedApplyAuto(bool fan, bool pump, bool dataStale);  // chi doi chap hanh khi dang Auto

void sharedSetNet(const NetState &net);
NetState sharedGetNet();
