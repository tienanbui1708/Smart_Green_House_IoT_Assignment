// node2_actuator/config.h
// Moi hang so cua Node 2. Khong chua token/mat khau (xem secrets.h).
#pragma once
#include <Arduino.h>

// ---- Firmware ----
#define FW_TITLE "greenhouse-node2"
#define FW_VERSION "1.0.0"
#define DEBUG_LOG 1

// Chua co ESP-NOW (M5): dung du lieu gia de thu hien thi va luat dieu khien.
#define USE_FAKE_SENSOR_DATA 1

// ---- Chan: macro cua variant yolo_uno ----
#define PIN_FAN_LED D13                   // LED onboard = quat mo phong
#define PIN_NEOPIXEL RGB_LED              // GPIO45 theo pins_arduino.h cua core
#define NEOPIXEL_COUNT 1                  // TODO(verify)
#define NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)  // TODO(verify)
#define NEOPIXEL_BRIGHTNESS 60

// ---- I2C ----
#define LCD_I2C_ADDR 0x21                 // TODO(verify): theo tools/i2c_scan
#define LCD_COLS 16
#define LCD_ROWS 2

// ---- Chu ky ----
#define CONTROL_PERIOD_MS 500
#define FAKE_DATA_PERIOD_MS 2000          // cung chu ky Node 1 gui
#define DISPLAY_PERIOD_MS 500
#define ACTUATOR_PERIOD_MS 200
#define STATS_PERIOD_MS 10000
#define DATA_STALE_MS 10000               // PROJECT.md 8.2: > 10 s la NO DATA

// ---- Luat dieu khien (PROJECT.md 8.3) ----
#define FAN_ON_T_ABOVE 32.0f
#define FAN_ON_H_ABOVE 80.0f
#define PUMP_ON_SOIL_BELOW 20
#define PUMP_OFF_SOIL_ABOVE 60
#define NEO_H_BLUE_BELOW 50.0f
#define NEO_H_RED_ABOVE 75.0f

// ---- Task ----
#define STACK_CONTROL 4096
#define STACK_DISPLAY 4096
#define STACK_ACTUATOR 4096
#define STACK_COREIOT 8192
#define STACK_STATS 3072
#define PRIO_CONTROL 3
#define PRIO_COREIOT 2
#define PRIO_DISPLAY 1
#define PRIO_ACTUATOR 1
#define PRIO_STATS 1

// ---- Mang (M3) ----
#define MQTT_CLIENT_ID "greenhouse-node2"
#define MQTT_BUFFER_SIZE 512
#define MQTT_KEEPALIVE_S 30
#define MQTT_SOCKET_TIMEOUT_S 5
#define MQTT_LOOP_MS 50
#define MQTT_RETRY_MS 5000
#define TELEMETRY_PERIOD_MS 5000
#define WIFI_ATTEMPT_TIMEOUT_MS 15000
#define WIFI_BACKOFF_LIST {3000, 5000, 10000, 20000, 30000}
