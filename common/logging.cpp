
#include <stdio.h>
#include <stdarg.h>
#include "logging.h"

static uint32_t gs_logLevel = LOG_NOTICE;

void LOG(uint32_t logLevel, const char *format, ...)
{
    if (logLevel > gs_logLevel)
    {
        return;
    }

    switch(logLevel)
    {
    case LOG_CRIT:
        printf("[CRIT] ");
        break;        
    case LOG_ERR:
        printf("[ERR ] ");
        break;
    case LOG_INFO:
        printf("[INFO] ");
        break;    
    case LOG_DEBUG:
        printf("[DBG ] ");
        break;                       
    default:
        break;
    }

    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void setLogLevel(uint32_t logLevel)
{
    gs_logLevel = logLevel;
}

uint32_t getLogLevel()
{
    return gs_logLevel;
}