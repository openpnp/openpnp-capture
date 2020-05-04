/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform device info object

    Created by Niels Moseley on 7/6/17.
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

#ifndef mac_platformdeviceinfo_h
#define mac_platformdeviceinfo_h

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
    uint16_t m_pid;         // USB product ID (if found) 
    uint16_t m_vid;         // USB vendor ID (if found)
    uint32_t m_busLocation; // 10-char hex bus location (if found)
};

#endif
