#include "../common/logging.h"
#include "platformstream.h"
#include "platformcontext.h"
#import <AVFoundation/AVFoundation.h>


// a platform factory function needed by
// libmain.cpp
Context* createPlatformContext()
{
    return new PlatformContext();
}

PlatformContext::PlatformContext() :
    Context()
{
    LOG(LOG_DEBUG, "Context created\n");
    enumerateDevices();
}

PlatformContext::~PlatformContext()
{
}

bool PlatformContext::enumerateDevices()
{
    for (AVCaptureDevice* device in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) {
        platformDeviceInfo* deviceInfo = new platformDeviceInfo();
        deviceInfo->m_uniqueId = std::string(device.uniqueID.UTF8String);
        deviceInfo->m_name = std::string(device.localizedName.UTF8String) + " " + deviceInfo->m_uniqueId;
        m_devices.push_back(deviceInfo);
    }
    
    return true;
}
