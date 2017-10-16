/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent library entry.

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

#define BUILD_OPENPNP_LIBRARY

#include "openpnp-capture.h"
#include "context.h"
#include "logging.h"
#include "version.h"

// Define a PlatformContext factory call 
// to separate platform dependent and independent
// code.
//
// This function must be implemented in platformcontext.cpp
Context* createPlatformContext();


DLLPUBLIC CapContext Cap_createContext()
{
    Context *ctx = createPlatformContext();
    return ctx;
}

DLLPUBLIC CapResult Cap_releaseContext(CapContext ctx)
{
    if (ctx != 0)
    {
        delete (Context*)ctx;
        return CAPRESULT_OK;
    }

    // context was NULL
    return CAPRESULT_ERR;
}

DLLPUBLIC uint32_t Cap_getDeviceCount(CapContext ctx)
{
    if (ctx != 0)
    {
        return reinterpret_cast<Context*>(ctx)->getDeviceCount();
    }
    return 0;
}

DLLPUBLIC const char* Cap_getDeviceName(CapContext ctx, CapDeviceID id)
{
    if (ctx != 0)
    {
        return reinterpret_cast<Context*>(ctx)->getDeviceName(id);
    }
    return 0;
}

DLLPUBLIC const char* Cap_getDeviceUniqueID(CapContext ctx, CapDeviceID id)
{
    if (ctx != 0)
    {
        return reinterpret_cast<Context*>(ctx)->getDeviceUniqueID(id);
    }
    return 0;    
}

DLLPUBLIC int32_t Cap_getNumFormats(CapContext ctx, CapDeviceID id)
{
    if (ctx != 0)
    {
        return reinterpret_cast<Context*>(ctx)->getNumFormats(id);
    }
    return -1;
}

DLLPUBLIC CapResult Cap_getFormatInfo(CapContext ctx, CapDeviceID index, CapFormatID id, CapFormatInfo *info)
{
    if (ctx != 0)
    {
        if (reinterpret_cast<Context*>(ctx)->getFormatInfo(index, id, info))
        {
            return CAPRESULT_OK;
        }
    }
    return CAPRESULT_ERR;    
}

DLLPUBLIC void Cap_setLogLevel(uint32_t level)
{
    setLogLevel(level);
}

DLLPUBLIC CapStream Cap_openStream(CapContext ctx, CapDeviceID index, CapFormatID formatID)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->openStream(index, formatID);
    }
    return -1;
}

DLLPUBLIC CapResult Cap_closeStream(CapContext ctx, CapStream stream)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        c->closeStream(stream);
    }
    return CAPRESULT_OK;
}

DLLPUBLIC uint32_t Cap_isOpenStream(CapContext ctx, CapStream stream)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->isOpenStream(stream);
    }
    return 0;   // closed stream
}

DLLPUBLIC CapResult Cap_captureFrame(CapContext ctx, CapStream stream, void *RGBbufferPtr, uint32_t RGBbufferBytes)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->captureFrame(stream, (uint8_t*)RGBbufferPtr, RGBbufferBytes) ? CAPRESULT_OK : CAPRESULT_ERR;
    }    
    return CAPRESULT_ERR;
}

DLLPUBLIC uint32_t Cap_hasNewFrame(CapContext ctx, CapStream stream)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->hasNewFrame(stream) ? 1: 0;
    }    
    return 0;
}

DLLPUBLIC uint32_t Cap_getStreamFrameCount(CapContext ctx, CapStream stream)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->getStreamFrameCount(stream);
    }    
    return 0;    
}

#if 0

// not used for now..

DLLPUBLIC CapResult Cap_setFrameRate(CapContext ctx, CapStream stream, uint32_t fps)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        if (!c->setStreamFrameRate(stream, fps))
        {
            return CAPRESULT_ERR;
        }
        return CAPRESULT_OK;
    }
    return CAPRESULT_ERR;    
}
#endif

DLLPUBLIC CapResult Cap_getPropertyLimits(CapContext ctx, CapStream stream, CapPropertyID propID, 
    int32_t *min, int32_t *max, int32_t *dValue)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        if (!c->getStreamPropertyLimits(stream, propID, min, max, dValue))
        {
            return CAPRESULT_PROPERTYNOTSUPPORTED;
        }
        return CAPRESULT_OK;
    }
    return CAPRESULT_ERR;
}

DLLPUBLIC CapResult Cap_setProperty(CapContext ctx, CapStream stream, CapPropertyID propID, int32_t value)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        if (!c->setStreamProperty(stream, propID, value))
        {
            return CAPRESULT_PROPERTYNOTSUPPORTED;
        }
        return CAPRESULT_OK;
    }
    return CAPRESULT_ERR;
}

DLLPUBLIC CapResult Cap_setAutoProperty(CapContext ctx, CapStream stream, CapPropertyID propID, uint32_t bOnOff)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        if (!c->setStreamAutoProperty(stream, propID, (bOnOff==1)))
        {
            return CAPRESULT_PROPERTYNOTSUPPORTED;
        }
        return CAPRESULT_OK;
    }
    return CAPRESULT_ERR;
}

DLLPUBLIC CapResult Cap_getProperty(CapContext ctx, CapStream stream, CapPropertyID propID, int32_t *outValue)
{
    if (outValue == NULL)
    {
        return CAPRESULT_ERR;
    }

    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        int32_t value = 0;
        if (!c->getStreamProperty(stream, propID, value))
        {
            return CAPRESULT_PROPERTYNOTSUPPORTED;
        }
        *outValue = value;
        return CAPRESULT_OK;
    }
    return CAPRESULT_ERR;
}

DLLPUBLIC CapResult Cap_getAutoProperty(CapContext ctx, CapStream stream, CapPropertyID propID, uint32_t *outValue)
{
    if (outValue == NULL)
    {
        return CAPRESULT_ERR;
    }

    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        bool enable = false;
        if (!c->getStreamAutoProperty(stream, propID, enable))
        {
            return CAPRESULT_PROPERTYNOTSUPPORTED;
        }
        *outValue = enable ? 1 : 0;
        return CAPRESULT_OK;
    }
    return CAPRESULT_ERR;
}

DLLPUBLIC void Cap_installCustomLogFunction(CapCustomLogFunc logFunc)
{
    installCustomLogFunction(logFunc);
}

DLLPUBLIC const char* Cap_getLibraryVersion()
{
    #ifndef __LIBVER__
    #define __LIBVER__ "VERSION UNKNOWN"
    #endif 
    
    #ifndef __PLATFORM__
    #define __PLATFORM__ "PLATFORM UNKNONW"
    #endif

    #ifndef __BUILDTYPE__
    #define __BUILDTYPE__ "BUILDTYPE UNKNOWN"
    #endif

    static const char versionString[] = __PLATFORM__ " " __BUILDTYPE__ " " __LIBVER__ " " __DATE__ " ";

    return versionString;
}