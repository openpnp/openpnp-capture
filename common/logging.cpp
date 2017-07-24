
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
        fprintf(stderr,"[CRIT] ");
        break;        
    case LOG_ERR:
        fprintf(stderr,"[ERR ] ");
        break;
    case LOG_INFO:
        fprintf(stderr,"[INFO] ");
        break;    
    case LOG_DEBUG:
        fprintf(stderr,"[DBG ] ");
        break;
    case LOG_VERBOSE:
        fprintf(stderr,"[VERB] ");
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