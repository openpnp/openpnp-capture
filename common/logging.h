
#ifndef logging_h
#define logging_h

#include <stdint.h>

// define log levels
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT  2
#define LOG_ERR   3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#define LOG_VERBOSE 8

void LOG(uint32_t logLevel, const char *format, ...);
void setLogLevel(uint32_t logLevel);
uint32_t getLogLevel();
#endif