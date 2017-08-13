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
};

#endif
