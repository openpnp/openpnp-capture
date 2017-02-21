#import <AVFoundation/AVFoundation.h>
#import "openpnp-capture.h"

@interface Context : NSObject
@property capture_device* devices;
@property size_t devices_length;
@end

@implementation Context

@end

capture_status create_context(capture_context** context_) {
    Context* context = [Context new];
    *context_ = (capture_context*) CFBridgingRetain(context);
    NSLog(@"create_context() -> %p", context);
    return CAPTURE_OK;
}

capture_status release_context(capture_context* context_) {
    NSLog(@"release_context(%p)", context_);
    Context* context = (__bridge Context*) (void*) context_;
    CFBridgingRelease(context_);
    return CAPTURE_OK;
}

capture_status list_devices(capture_context* context_, capture_device** devices, unsigned int* devices_length) {
    // The extra cast to void* seems to be required to make the compiler happy,
    // and I got that solution from http://stackoverflow.com/questions/10273833/casting-unsafe-unretained-id-to-const-void.
    Context* context = (__bridge Context*) (void*) context_;
    NSLog(@"list_devices(%p -> %p, %p, %p)", context_, context, devices, devices_length);

    if (context.devices) {
        free(context.devices);
        context.devices = NULL;
        context.devices_length = 0;
    }
    
    unsigned long count = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo].count;
    context.devices_length = count;
    context.devices = malloc(count * sizeof(capture_device));
    for (int i = 0; i < count; i++) {
        AVCaptureDevice* device = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo][i];
        context.devices[i]._internal = (__bridge void *)(device);
        context.devices[i].name = device.localizedName.UTF8String;
        context.devices[i].unique_id = device.uniqueID.UTF8String;
        context.devices[i].manufacturer = device.manufacturer.UTF8String;
        context.devices[i].model = device.modelID.UTF8String;
    }
    *devices = context.devices;
    *devices_length = context.devices_length;
    return CAPTURE_OK;
}


//wchar_t* say_hello() {
//    for (AVCaptureDevice *device in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) {
//        NSLog(@"%@", device);
//        NSLog(@"localizedName %@, modelID %@, uniqueID %@", device.localizedName, device.modelID, device.uniqueID);
//        NSLog(@"  AVCaptureExposureModeLocked %hhd, AVCaptureExposureModeAutoExpose %hhd, AVCaptureExposureModeContinuousAutoExposure %hhd",
//              [device isExposureModeSupported:AVCaptureExposureModeLocked],
//              [device isExposureModeSupported:AVCaptureExposureModeAutoExpose],
//              [device isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure]
//              );
//        NSLog(@"  AVCaptureFocusModeLocked %hhd, AVCaptureFocusModeAutoFocus %hhd, AVCaptureFocusModeContinuousAutoFocus %hhd",
//              [device isFocusModeSupported:AVCaptureFocusModeLocked],
//              [device isFocusModeSupported:AVCaptureFocusModeAutoFocus],
//              [device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]
//              );
//        for (AVCaptureDeviceFormat *format in [device formats]) {
//            NSLog(@"  %@", format);
//            //            NSLog(@"    %@", format.formatDescription);
//            NSLog(@"    %@", format.videoSupportedFrameRateRanges);
//            NSLog(@"    %@ %u",
//                  NSFileTypeForHFSTypeCode(CMFormatDescriptionGetMediaSubType(format.formatDescription)),
//                  (unsigned int) format.formatDescription);
//            CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
//            NSLog(@"    %d, %d", dims.width, dims.height);
//        }
//    }
//    return L"hi there";
//}
