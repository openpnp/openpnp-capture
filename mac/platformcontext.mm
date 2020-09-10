/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform context class

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

#include "../common/logging.h"
#include "platformstream.h"
#include "platformcontext.h"
#import <AVFoundation/AVFoundation.h>

#include <chrono>
#include <thread>

// a platform factory function needed by
// libmain.cpp
Context* createPlatformContext()
{
    return new PlatformContext();
}

PlatformContext::PlatformContext() :
    Context()
{
    LOG(LOG_INFO, "Platform context created\n");
    if ([AVCaptureDevice respondsToSelector:@selector(authorizationStatusForMediaType:)]) {
        cameraPermissionReceived = 0;
        if ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo] == AVAuthorizationStatusAuthorized) {
            NSLog(@"Already have camera permission");
            cameraPermissionReceived = 1;
        }
        else {
            NSLog(@"Requesting permission, bundle path for Info.plist: %@", [[NSBundle mainBundle] bundlePath]);
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                if (granted) {
                    cameraPermissionReceived = 1;
                } else {
                    cameraPermissionReceived = -1;
                }
                if (granted) {
                    NSLog(@"Permission granted");
                } else {
                    NSLog(@"Failed to get permission");
                }
            } ];
            while (cameraPermissionReceived == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        if (cameraPermissionReceived == 1) {
            enumerateDevices();
        }
    }
    else {
        enumerateDevices();
    }
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

        std::string model = device.modelID.UTF8String;
        LOG(LOG_DEBUG, "Name : %s\n", deviceInfo->m_name.c_str());
        LOG(LOG_DEBUG, "Model: %s\n", model.c_str());
        LOG(LOG_DEBUG, "U ID : %s\n", deviceInfo->m_uniqueID.c_str());

        // extract the PID/VID from the model name
        NSRange vidRange = [device.modelID rangeOfString:@"VendorID_"];
        if (vidRange.length > 0)
        {
            uint32_t maxLen = device.modelID.length - vidRange.location - 9;
            maxLen = (maxLen > 5) ? 5 : maxLen;        
            deviceInfo->m_vid = [[device.modelID substringWithRange:NSMakeRange(vidRange.location + 9, maxLen)] intValue];
        }
        else
        {
            LOG(LOG_WARNING, "OSX Unable to extract vendor ID\n");
        }
        

        NSRange pidRange = [device.modelID rangeOfString:@"ProductID_"];
        if (pidRange.length > 0)
        {
            uint32_t maxLen = device.modelID.length - pidRange.location - 10;
            maxLen = (maxLen > 5) ? 5 : maxLen;
            deviceInfo->m_pid = [[device.modelID substringWithRange:NSMakeRange(pidRange.location + 10, maxLen)] intValue];
        }
        else
        {
            LOG(LOG_WARNING, "OSX Unable to extract product ID\n");
        }
        

        LOG(LOG_DEBUG, "USB      : vid=%04X  pid=%04X\n", deviceInfo->m_vid, deviceInfo->m_pid);

        // the unique ID seem to be comprised of a 10-character PCI/USB location address
        // followed by the VID and PID in hex, e.g. 0x26210000046d0825
        deviceInfo->m_busLocation = 0;
        if (device.uniqueID.length == 18)
        {
            std::string locStdStr = std::string(device.uniqueID.UTF8String);
            
            // sanity check for PID and VID to make sure the unique ID is indeed
            // in the format we expect..
            char pidStr[5];
            char vidStr[5];
            snprintf(pidStr, sizeof(pidStr), "%04x", deviceInfo->m_pid);
            snprintf(vidStr, sizeof(vidStr), "%04x", deviceInfo->m_vid);

            if ((locStdStr.substr(10,4) == std::string(vidStr)) &&
                (locStdStr.substr(14,4) == std::string(pidStr)))
            {
                // format seems to be correct
                NSString *hexString = [device.uniqueID substringWithRange:NSMakeRange(2,8)];
                NSScanner *scanner = [NSScanner scannerWithString:hexString];
                [scanner scanHexInt:&(deviceInfo->m_busLocation)];

                LOG(LOG_DEBUG, "Location : %08X\n", deviceInfo->m_busLocation);
                [scanner dealloc];
                [hexString dealloc];
            }
            else
            {
                LOG(LOG_DEBUG, "VID/PID mismatch!\n");
                LOG(LOG_DEBUG, "Extracted VID %s\n", locStdStr.substr(10,4).c_str());
                LOG(LOG_DEBUG, "Extracted PID %s\n", locStdStr.substr(14,4).c_str());
            }
        }
        
        if (deviceInfo->m_busLocation == 0)
        {
            LOG(LOG_WARNING, "OSX Unique ID is not exactly 18 characters - wrong format to extract location.\n");
            LOG(LOG_WARNING, "We might have trouble identifying the UVC control interface.\n");
        }

        for (AVCaptureDeviceFormat* format in device.formats) 
        {
            //Do we really need a complete list of frame rates?
            //Hopefully, we can search for a suitable frame rate
            //when we open the device later...
            //
            //This is more in line with the Windows and Linux
            //versions.
            //
            // For now, just report the max frame rate

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
            #endif

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
        }
        
        m_devices.push_back(deviceInfo);
    }
    
    return true;
}

