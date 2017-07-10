/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Niels Moseley

*/

#define BUILD_OPENPNP_LIBRARY

#include "openpnp-capture.h"
#include "context.h"
#include "logging.h"

DLLPUBLIC CapContext Cap_createContext()
{
    Context *ctx = new Context();
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

DLLPUBLIC void Cap_setLogLevel(uint32_t level)
{
    setLogLevel(level);
}

DLLPUBLIC CapStream Cap_openStream(CapContext ctx, CapDeviceID index, uint32_t width, uint32_t height, uint32_t fourCC)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->openStream(index);
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