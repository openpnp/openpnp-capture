#include "platformdeviceinfo.h"
#include "platformstream.h"
#include "platformcontext.h"

Stream* createPlatformStream()
{
    return new PlatformStream();
}

PlatformStream::PlatformStream() :
    Stream()
{

}

PlatformStream::~PlatformStream()
{
    close();
}

void PlatformStream::close()
{
    LOG(LOG_INFO, "closing stream\n");
}

bool PlatformStream::open(Context *owner, deviceInfo *device, uint32_t width, uint32_t height, uint32_t fourCC)
{
    if (m_isOpen)
    {
        LOG(LOG_INFO,"open() was called on an active stream.\n");
        close();
    }

    if (owner == nullptr)
    {
        LOG(LOG_ERR,"open() was with owner=NULL!\n");        
        return false;
    }

    if (device == nullptr)
    {
        LOG(LOG_ERR,"open() was with device=NULL!\n");
        return false;
    }

    platformDeviceInfo *dinfo = dynamic_cast<platformDeviceInfo*>(device);
    if (dinfo == NULL)
    {
        LOG(LOG_CRIT, "Could not cast deviceInfo* to platfromDeviceInfo*!");
        return false;
    }

    return true;
}

uint32_t PlatformStream::getFOURCC()
{
    return 0;
}


bool PlatformStream::setExposure(int32_t value) 
{
    return false;
}


bool PlatformStream::setAutoExposure(bool enabled) 
{
    return false;
}


bool PlatformStream::getExposureLimits(int32_t *emin, int32_t *emax) 
{
    return false;
}

std::string PlatformStream::genFOURCCstring(uint32_t v)
{
    std::string result;
    for(uint32_t i=0; i<4; i++)
    {
        result += static_cast<char>(v & 0xFF);
        v >>= 8;
    }
    return result;
}

