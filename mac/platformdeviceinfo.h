/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform device info object

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley, Jason von Nieda.

*/

#ifndef platformdeviceinfo_h
#define platformdeviceinfo_h

#include <string>
#include <vector>

#include "../common/deviceinfo.h"
#include "../common/logging.h"
#import <AVFoundation/AVFoundation.h>

/** device information struct/object */
class platformDeviceInfo : public deviceInfo
{
public:
    platformDeviceInfo() : deviceInfo() 
    {
        m_captureDevice = nullptr;
        m_pid = 0;
        m_vid = 0;
    }

    virtual ~platformDeviceInfo()
    {
        //LOG(LOG_DEBUG, "%s\n", CFStringGetCStringPtr(CFCopyTypeIDDescription(CFGetTypeID(m_captureDevice)),kCFStringEncodingUTF8));
        if (m_captureDevice != nullptr)
        {
            LOG(LOG_DEBUG, "m_captureDevice released\n");
            CFRelease(m_captureDevice);
        }
        #if 0
        auto iter = m_platformFormats.begin();
        while(iter != m_platformFormats.end())
        {
            delete *iter;
            iter++;
        }
        #endif
    }

    CFTypeRef m_captureDevice;   ///< pointer to AVCaptureDevice
    std::vector<AVCaptureDeviceFormat*> m_platformFormats; ///< formats supported by the device

    // save USB PID/VID so we can access the UVC camera
    // directly for setting focus/exposure etc.
    uint16_t m_pid; // USB product ID (if found) 
    uint16_t m_vid; // USB vendor ID (if found)
};

#endif
