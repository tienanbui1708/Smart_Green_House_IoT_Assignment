// node1_sensor/task_sensor.cpp
// Doc DHT20 + do am dat moi SENSOR_PERIOD_MS, ghi vao shared_data.
#include <Wire.h>
#include "DHT20.h"
#include <app_log.h>
#include <protocol.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
constexpr int RC_I2C_BUSY = -100;  // ma rieng: khong lay duoc mutex I2C
DHT20 dht(&Wire);

uint8_t soilPercentFromRaw(uint16_t raw) {
  static_assert(SOIL_RAW_DRY != SOIL_RAW_WET, "SOIL_RAW_DRY va SOIL_RAW_WET phai khac nhau");
  long pct = ((long)raw - SOIL_RAW_DRY) * 100L / ((long)SOIL_RAW_WET - SOIL_RAW_DRY);
  return (uint8_t)constrain(pct, 0L, 100L);
}

uint16_t readSoilRaw() {
  uint32_t sum = 0;
  for (int i = 0; i < SOIL_SAMPLES; i++) {
    sum += analogRead(PIN_SOIL);
    vTaskDelay(pdMS_TO_TICKS(SOIL_SAMPLE_GAP_MS));
  }
  return (uint16_t)(sum / SOIL_SAMPLES);
}

// Tach request/read thay vi dht.read(): read() cho ban ~80 ms, se giu mutex I2C va chan LCD.
int readDht(float &t, float &h) {
  if (!i2cLock(I2C_LOCK_TIMEOUT_MS)) return RC_I2C_BUSY;
  int rc = dht.requestData();
  i2cUnlock();
  if (rc != DHT20_OK) return rc;

  vTaskDelay(pdMS_TO_TICKS(DHT20_MEASURE_MS));

  if (!i2cLock(I2C_LOCK_TIMEOUT_MS)) return RC_I2C_BUSY;
  rc = dht.isMeasuring() ? DHT20_ERROR_READ_TIMEOUT : dht.readData();
  if (rc == DHT20_OK) rc = dht.convert();
  i2cUnlock();
  if (rc != DHT20_OK) return rc;

  t = dht.getTemperature();
  h = dht.getHumidity();
  return DHT20_OK;
}

bool inValidRange(float t, float h) {
  return t >= DHT20_T_MIN && t <= DHT20_T_MAX && h >= DHT20_H_MIN && h <= DHT20_H_MAX;
}

SensorSample takeSample(uint32_t seq, int &rc) {
  SensorSample s = {};
  float t = 0, h = 0;
  rc = readDht(t, h);
  s.dhtOk = (rc == DHT20_OK) && inValidRange(t, h);
  s.temperature = s.dhtOk ? t : SENTINEL_T;  // quy tac sentinel TINYML_GUIDE 2.3
  s.humidity = s.dhtOk ? h : SENTINEL_H;
  s.soilRaw = readSoilRaw();
  s.soil = soilPercentFromRaw(s.soilRaw);
  s.seq = seq;
  s.takenMs = millis();
  return s;
}
}  // namespace

void taskSensor(void *) {
  analogReadResolution(ADC_BITS);
  uint32_t seq = 0;
  uint32_t failStreak = 0;
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    int rc = DHT20_OK;
    SensorSample s = takeSample(++seq, rc);
    sharedSetSample(s);

    if (!s.dhtOk) {
      appLog("SENSOR", "DHT20 read failed rc=%d streak=%lu", rc, (unsigned long)++failStreak);
    } else if (failStreak > 0) {
      appLog("SENSOR", "DHT20 recovered after %lu failures", (unsigned long)failStreak);
      failStreak = 0;
    }
#if DEBUG_LOG
    appLog("SENSOR", "seq=%lu T=%.1f H=%.1f soil=%u%% raw=%u", (unsigned long)s.seq,
           s.temperature, s.humidity, s.soil, s.soilRaw);
#endif
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
  }
}
