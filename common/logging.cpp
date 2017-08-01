/**

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