/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Niels Moseley

*/

#define BUILD_OPENPNP_LIBRARY

#include "openpnp-capture.h"
#include "context.h"
#include "logging.h"

DLLEXPORT CapContext Cap_createContext()
{
    Context *ctx = new Context();
    return ctx;
}

DLLEXPORT CapResult Cap_releaseContext(CapContext ctx)
{
    if (ctx != 0)
    {
        delete (Context*)ctx;
        return CAPRESULT_OK;
    }

    // context was NULL
    return CAPRESULT_ERR;
}

DLLEXPORT uint32_t Cap_getDeviceCount(CapContext ctx)
{
    if (ctx != 0)
    {
        return reinterpret_cast<Context*>(ctx)->getDeviceCount();
    }
    return 0;
}

DLLEXPORT const char* Cap_getDeviceName(CapContext ctx, CapDeviceID id)
{
    if (ctx != 0)
    {
        return reinterpret_cast<Context*>(ctx)->getDeviceName(id);
    }
    return 0;
}

DLLEXPORT void Cap_setLogLevel(uint32_t level)
{
    setLogLevel(level);
}

DLLEXPORT CapStream Cap_openStream(CapContext ctx, CapDeviceID index, uint32_t width, uint32_t height, uint32_t fourCC)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        return c->openStream(index);
    }
    return -1;
}

DLLEXPORT CapResult Cap_closeStream(CapContext ctx, CapStream stream)
{
    if (ctx != 0)
    {
        Context *c = reinterpret_cast<Context*>(ctx);
        c->closeStream(stream);
    }
    return CAPRESULT_OK;
}