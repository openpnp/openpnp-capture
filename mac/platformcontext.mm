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
    LOG(LOG_DEBUG, "Platform context created\n");
    enumerateDevices();
}

PlatformContext::~PlatformContext()
{
    LOG(LOG_DEBUG, "Platform context destroyed\n");
}

bool PlatformContext::enumerateDevices()
{
    LOG(LOG_DEBUG, "enumerateDevices called\n");

    m_devices.clear();
    for (AVCaptureDevice* device in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) 
    {
        platformDeviceInfo* deviceInfo = new platformDeviceInfo();
        deviceInfo->m_captureDevice = CFBridgingRetain(device);
        
        deviceInfo->m_name = std::string(device.localizedName.UTF8String) + " (" + std::string(device.manufacturer.UTF8String) + ")";
        deviceInfo->m_uniqueID = deviceInfo->m_name  + " " + std::string(device.uniqueID.UTF8String);
        
        LOG(LOG_DEBUG, "Name: %s\n", deviceInfo->m_name.c_str());
        LOG(LOG_DEBUG, "ID  : %s\n", deviceInfo->m_uniqueID.c_str());

        for (AVCaptureDeviceFormat* format in device.formats) 
        {
            //Do we really need a complete list of frame rates?
            //Hopefully, we can search for a suitable frame rate
            //when we open the device later...
            //
            //This is more in line with the Windows and Linux
            //versions
            
            #if 0
            for (AVFrameRateRange* frameRateRange in format.videoSupportedFrameRateRanges) {
                for (int frameRate = frameRateRange.minFrameRate; frameRate <= frameRateRange.maxFrameRate; frameRate++) {
                    CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
                    CapFormatInfo formatInfo;
                    formatInfo.width = dims.width;
                    formatInfo.height = dims.height;
                    formatInfo.fourcc = CMFormatDescriptionGetMediaSubType(format.formatDescription);
                    formatInfo.fps = frameRate;
                    deviceInfo->m_formats.push_back(formatInfo);
                }
            }
            #else

            CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            CapFormatInfo formatInfo;
            formatInfo.width = dims.width;
            formatInfo.height = dims.height;
            formatInfo.fourcc = CMFormatDescriptionGetMediaSubType(format.formatDescription);
            
            uint32_t maxFrameRate = 0;
            for (AVFrameRateRange* frameRateRange in format.videoSupportedFrameRateRanges) 
            {
                // find max frame rate
                if (maxFrameRate < frameRateRange.maxFrameRate)
                {
                    maxFrameRate = frameRateRange.maxFrameRate;
                }
            }
            formatInfo.fps = maxFrameRate; // just use maximum for now!
            deviceInfo->m_formats.push_back(formatInfo);
            deviceInfo->m_platformFormats.push_back(format);
            #endif
        }
        
        m_devices.push_back(deviceInfo);
    }
    
    return true;
}

