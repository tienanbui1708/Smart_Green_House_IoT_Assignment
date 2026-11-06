// node1_sensor/tasks.h
#pragma once

void taskSensor(void *arg);
void taskDisplay(void *arg);
void taskLed(void *arg);
void taskCoreiot(void *arg);
void taskEspnow(void *arg);
void taskWeb(void *arg);
void coreiotLoadConfig();  // goi trong setup(): /config.json hoac secrets.h
