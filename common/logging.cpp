/*

    Logging subsystem for OpenPnP Capture library.

    Copyright (c) 2017 Jason von Nieda, Niels Moseley.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <stdio.h>
#include <stdarg.h>
#include "logging.h"

/* In their infinite "wisdom" Microsoft have declared snprintf is deprecated
   and we must therefore resort to a macro to fix something that shouldn't
   be a problem */
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

static uint32_t gs_logLevel = LOG_NOTICE;
static customLogFunc gs_logFunc = NULL;

void installCustomLogFunction(customLogFunc logfunc)
{
    gs_logFunc = logfunc;
}

void LOG(uint32_t logLevel, const char *format, ...)
{
    if (logLevel > gs_logLevel)
    {
        return;
    }

    char logbuffer[1024];
    char *ptr = logbuffer;

    switch(logLevel)
    {
    case LOG_CRIT:
        snprintf(logbuffer,1024,"[CRIT] ");
        ptr += 7;
        break;        
    case LOG_ERR:
        snprintf(logbuffer,1024,"[ERR ] ");
        ptr += 7;
        break;
    case LOG_INFO:
        snprintf(logbuffer,1024,"[INFO] ");
        ptr += 7;
        break;    
    case LOG_DEBUG:
        snprintf(logbuffer,1024,"[DBG ] ");
        ptr += 7;
        break;
    case LOG_VERBOSE:
        snprintf(logbuffer,1024,"[VERB] ");
        ptr += 7;
        break;
    default:
        break;
    }
    
    va_list args;

    va_start(args, format);
    vsnprintf(ptr, 1024-7, format, args);
    va_end(args);

    if (gs_logFunc != nullptr)
    {
        // custom log functions to no include the 
        // prefix.
        gs_logFunc(logLevel, ptr);
    }
    else
    {
        fprintf(stderr, "%s", logbuffer);
    }
}

void setLogLevel(uint32_t logLevel)
{
    gs_logLevel = logLevel;
}

uint32_t getLogLevel()
{
    return gs_logLevel;
}