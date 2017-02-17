#import <AVFoundation/AVFoundation.h>
#import "openpnp-capture.h"

@interface Context : NSObject
@property capture_device** devices;
@end

@implementation Context
@end

capture_context create_context() {
    Context* context = [Context new];
    return (capture_context) CFBridgingRetain(context);
}

void release_context(capture_context context) {
    CFBridgingRelease(context);
}

capture_device** list_devices(capture_context context) {
    Context* context_ = (__bridge Context*) context;
    
    if (context_.devices) {
        for (int i = 0; context_.devices[i]; i++) {
            free(context_.devices[i]);
        }
        free(context_.devices);
        context_.devices = NULL;
    }
    
    unsigned long count = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo].count;
    context_.devices = malloc((count + 1) * sizeof(capture_device*));
    context_.devices[count] = NULL;
    for (int i = 0; i < count; i++) {
        AVCaptureDevice* device = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo][i];
        context_.devices[i] = malloc(sizeof(capture_device));
        context_.devices[i]->_internal = (__bridge void *)(device);
        context_.devices[i]->name = device.localizedName.UTF8String;
        context_.devices[i]->unique_id = device.uniqueID.UTF8String;
        context_.devices[i]->manufacturer = device.manufacturer.UTF8String;
        context_.devices[i]->model = device.modelID.UTF8String;
    }
    return context_.devices;
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
