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

enum class CmdResult : uint8_t { Ok, RejectedAutoMode };

void sharedInit();  // goi trong setup() truoc khi tao task

void sharedSetSample(const SensorSample &sample);
SensorSample sharedGetSample();

ControlState sharedGetControl();
void sharedSetAutoMode(bool autoMode);
CmdResult sharedSetManualHeater(uint8_t value);
void sharedSetAutoHeater(uint8_t value);  // bo qua neu dang Manual
bool sharedForceSafe();                   // ep Auto + tat suoi; tra true neu trang thai doi

void sharedSetNet(const NetState &net);
NetState sharedGetNet();

// Bus I2C dung chung cho DHT20 (task sensor) va LCD (task display).
bool i2cLock(uint32_t timeoutMs);
void i2cUnlock();
