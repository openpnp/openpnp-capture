#import <AVFoundation/AVFoundation.h>
#import "openpnp-capture.h"

@interface Context : NSObject
@property capture_device* devices;
@property size_t devices_length;
@end

// TODO pick a naming scheme and stick with it
// Thinking native_x, x. Refactor all.

@implementation Context
- (void) listDevices {
    [self freeDevices];
    
    self.devices_length = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo].count;
    self.devices = malloc(self.devices_length * sizeof(capture_device));
    for (int i = 0; i < self.devices_length; i++) {
        AVCaptureDevice* native_device = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo][i];
        capture_device* device = &self.devices[i];
        device->_internal = CFBridgingRetain(native_device);
        device->name = native_device.localizedName.UTF8String;
        device->unique_id = native_device.uniqueID.UTF8String;
        device->manufacturer = native_device.manufacturer.UTF8String;
        device->model = native_device.modelID.UTF8String;
        
        device->supportsExposureAuto = [native_device isExposureModeSupported:AVCaptureExposureModeAutoExpose] || [native_device isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure];
        device->supportsExposureManual = [native_device isExposureModeSupported:AVCaptureExposureModeLocked];
        device->supportsFocusAuto = [native_device isFocusModeSupported:AVCaptureExposureModeAutoExpose] || [native_device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus];
        device->supportsFocusManual = [native_device isFocusModeSupported:AVCaptureFocusModeLocked];

        device->formats_length = native_device.formats.count;
        device->formats = malloc(device->formats_length * sizeof(capture_format));
        
        int formatIndex = 0;
        for (AVCaptureDeviceFormat* native_format in native_device.formats) {
            for (AVFrameRateRange* native_frame_rate_range in native_format.videoSupportedFrameRateRanges) {
                for (int native_frame_rate = native_frame_rate_range.minFrameRate; native_frame_rate <= native_frame_rate_range.maxFrameRate; native_frame_rate++) {
                    if (formatIndex >= device->formats_length) {
                        device->formats_length++;
                        device->formats = realloc(device->formats, device->formats_length * sizeof(capture_format));
                    }
                    CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(native_format.formatDescription);
                    device->formats[formatIndex]._internal = CFBridgingRetain(native_format);
                    device->formats[formatIndex].fps = native_frame_rate;
                    device->formats[formatIndex].fourcc = CMFormatDescriptionGetMediaSubType(native_format.formatDescription);
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
            CFBridgingRelease(self.devices[i]._internal);
            for (int j = 0; j < self.devices[i].formats_length; j++) {
                CFBridgingRelease(self.devices[i].formats[j]._internal);
            }
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

capture_status create_context(capture_context** context) {
    Context* native_context = [Context new];
    *context = (capture_context*) CFBridgingRetain(native_context);
    return CAPTURE_OK;
}

capture_status release_context(capture_context* context) {
    Context* native_context = (__bridge Context*) (void*) context;
    CFBridgingRelease(context);
    return CAPTURE_OK;
}

capture_status list_devices(capture_context* context, capture_device** devices, unsigned int* devices_length) {
    Context* native_context = (__bridge Context*) (void*) context;

    [native_context listDevices];
    
    *devices = native_context.devices;
    *devices_length = native_context.devices_length;
    return CAPTURE_OK;
}

capture_status open_session(capture_device* device, capture_session** session) {
    AVCaptureDevice* native_device = (__bridge AVCaptureDevice*) (void*) device->_internal;

    NSLog(@"open_session(%@, %p)", native_device, session);
    
    AVCaptureSession* native_session = [AVCaptureSession new];
    
    NSError* error = nil;
    AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:native_device error:&error];
    if (!input) {
        NSLog(@"open_session() error: %@", error);
        return CAPTURE_ERROR;
    }
    
    AVCaptureVideoDataOutput* output = [AVCaptureVideoDataOutput new];
    
    [native_session addInput:input];
    [native_session addOutput:output];
    
    // TODO set delegate and figure out what to do with frames
    // TODO remove this, maybe? Not sure if we do this here yet or externally.
    [native_session startRunning];
    
    *session = CFBridgingRetain(native_session);
    return CAPTURE_OK;
}

capture_status close_session(capture_session* session) {
    return CAPTURE_OK;
}
