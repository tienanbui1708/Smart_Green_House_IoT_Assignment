// node2_actuator/tasks.h
#pragma once

void taskControl(void *arg);
void taskDisplay(void *arg);
void taskActuator(void *arg);
void taskCoreiot(void *arg);
void taskEspnowRx(void *arg);
void taskWeb(void *arg);
void coreiotLoadConfig();  // goi trong setup(): /config.json hoac secrets.h
