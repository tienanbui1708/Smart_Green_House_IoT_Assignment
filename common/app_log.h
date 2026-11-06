// common/app_log.h
// Log Serial co tien to task, qua mutex de nhieu task in khong chen nhau.
#pragma once

void logInit();
void appLog(const char *tag, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
// Dong khong tien to (vd 'D,...' cho ml/collect_serial.py), cung mutex de khong chen vao dong khac.
void appLogRaw(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
