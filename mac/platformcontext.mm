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
        
        for (AVCaptureDeviceFormat* format in device.formats) {
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
