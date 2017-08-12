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

    for (AVCaptureDevice* device in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) 
    {
        platformDeviceInfo* deviceInfo = new platformDeviceInfo();
        deviceInfo->m_captureDevice = CFBridgingRetain(device);
        
        deviceInfo->m_uniqueID = std::string(device.localizedName.UTF8String) + " " + std::string(device.uniqueID.UTF8String);
        deviceInfo->m_name = std::string(device.localizedName.UTF8String);
        
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
            #endif
        }
        
        m_devices.push_back(deviceInfo);
    }
    
    return true;
}

//- (void) listDevices {
//    [self freeDevices];
//    
//    self.devices_length = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo].count;
//    self.devices = malloc(self.devices_length * sizeof(capture_device));
//    for (int i = 0; i < self.devices_length; i++) {
//        AVCaptureDevice* native_device = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo][i];
//        capture_device* device = &self.devices[i];
//        device->_internal = CFBridgingRetain(native_device);
//        // TODO these aren't safe
//        device->name = native_device.localizedName.UTF8String;
//        device->unique_id = native_device.uniqueID.UTF8String;
//        device->manufacturer = native_device.manufacturer.UTF8String;
//        device->model = native_device.modelID.UTF8String;
//        
//        device->supportsExposureAuto = [native_device isExposureModeSupported:AVCaptureExposureModeAutoExpose] || [native_device isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure];
//        device->supportsExposureManual = [native_device isExposureModeSupported:AVCaptureExposureModeLocked];
//        device->supportsFocusAuto = [native_device isFocusModeSupported:AVCaptureExposureModeAutoExpose] || [native_device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus];
//        device->supportsFocusManual = [native_device isFocusModeSupported:AVCaptureFocusModeLocked];
//        
//        device->formats_length = native_device.formats.count;
//        device->formats = malloc(device->formats_length * sizeof(capture_format));
//        
//        int formatIndex = 0;
//        for (AVCaptureDeviceFormat* native_format in native_device.formats) {
//            for (AVFrameRateRange* native_frame_rate_range in native_format.videoSupportedFrameRateRanges) {
//                for (int native_frame_rate = native_frame_rate_range.minFrameRate; native_frame_rate <= native_frame_rate_range.maxFrameRate; native_frame_rate++) {
//                    if (formatIndex >= device->formats_length) {
//                        device->formats_length++;
//                        device->formats = realloc(device->formats, device->formats_length * sizeof(capture_format));
//                    }
//                    CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(native_format.formatDescription);
//                    device->formats[formatIndex]._internal = CFBridgingRetain(native_format);
//                    device->formats[formatIndex].fps = native_frame_rate;
//                    device->formats[formatIndex].fourcc = CMFormatDescriptionGetMediaSubType(native_format.formatDescription);
//                    device->formats[formatIndex].width = dims.width;
//                    device->formats[formatIndex].height = dims.height;
//                    formatIndex++;
//                }
//            }
//        }
//    }
//}
