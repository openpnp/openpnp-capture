#import <AVFoundation/AVFoundation.h>
#import "openpnp-capture.h"

@interface Context : NSObject
@property capture_device* devices;
@property size_t devices_length;
@end

@implementation Context
- (void) listDevices {
    [self freeDevices];
    
    self.devices_length = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo].count;
    self.devices = malloc(self.devices_length * sizeof(capture_device));
    for (int i = 0; i < self.devices_length; i++) {
        AVCaptureDevice* nativeDevice = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo][i];
        capture_device* device = &self.devices[i];
        device->_internal = (__bridge void *)(nativeDevice);
        device->name = nativeDevice.localizedName.UTF8String;
        device->unique_id = nativeDevice.uniqueID.UTF8String;
        device->manufacturer = nativeDevice.manufacturer.UTF8String;
        device->model = nativeDevice.modelID.UTF8String;
        
        device->supportsExposureAuto = [nativeDevice isExposureModeSupported:AVCaptureExposureModeAutoExpose] || [nativeDevice isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure];
        device->supportsExposureManual = [nativeDevice isExposureModeSupported:AVCaptureExposureModeLocked];
        device->supportsFocusAuto = [nativeDevice isFocusModeSupported:AVCaptureExposureModeAutoExpose] || [nativeDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus];
        device->supportsFocusManual = [nativeDevice isFocusModeSupported:AVCaptureFocusModeLocked];

        device->formats_length = nativeDevice.formats.count;
        device->formats = malloc(device->formats_length * sizeof(capture_format));
        
        int formatIndex = 0;
        for (AVCaptureDeviceFormat* format in nativeDevice.formats) {
            for (AVFrameRateRange* frr in format.videoSupportedFrameRateRanges) {
                for (int fr = frr.minFrameRate; fr <= frr.maxFrameRate; fr++) {
                    if (formatIndex >= device->formats_length) {
                        device->formats_length++;
                        device->formats = realloc(device->formats, device->formats_length * sizeof(capture_format));
                    }
                    CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
                    device->formats[formatIndex].fps = fr;
                    device->formats[formatIndex].fourcc = CMFormatDescriptionGetMediaSubType(format.formatDescription);
                    device->formats[formatIndex].width = dims.width;
                    device->formats[formatIndex].height = dims.height;
                    formatIndex++;
                }
            }
        }
    }
}

- (void) freeDevices {
    if (self.devices) {
        for (int i = 0; i < self.devices_length; i++) {
            free(self.devices[i].formats);
            self.devices[i].formats = NULL;
            self.devices[i].formats_length = 0;
        }
        free(self.devices);
        self.devices = NULL;
        self.devices_length = 0;
    }
}

- (void) dealloc {
    [self freeDevices];
}
@end

capture_status create_context(capture_context** context_) {
    Context* context = [Context new];
    *context_ = (capture_context*) CFBridgingRetain(context);
    return CAPTURE_OK;
}

capture_status release_context(capture_context* context_) {
    Context* context = (__bridge Context*) (void*) context_;
    CFBridgingRelease(context_);
    return CAPTURE_OK;
}

capture_status list_devices(capture_context* context_, capture_device** devices, unsigned int* devices_length) {
    Context* context = (__bridge Context*) (void*) context_;

    [context listDevices];
    
    *devices = context.devices;
    *devices_length = context.devices_length;
    return CAPTURE_OK;
}
