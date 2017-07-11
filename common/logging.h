
#ifndef logging_h
#define logging_h

#include <stdint.h>

/*
       Kernel constant   Level value   Meaning
       KERN_EMERG             0        System is unusable
       KERN_ALERT             1        Action must be taken immediately
       KERN_CRIT              2        Critical conditions
       KERN_ERR               3        Error conditions
       KERN_WARNING           4        Warning conditions
       KERN_NOTICE            5        Normal but significant condition
       KERN_INFO              6        Informational
       KERN_DEBUG             7        Debug-level messages
*/

// define log levels
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT  2
#define LOG_ERR   3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

void LOG(uint32_t logLevel, const char *format, ...);
void setLogLevel(uint32_t logLevel);
uint32_t getLogLevel();
#endif