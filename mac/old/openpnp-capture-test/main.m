//
//  main.m
//  openpnp-capture-test
//
//  Created by Jason von Nieda on 2/15/17.
//  Copyright © 2017 Jason von Nieda. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "openpnp-capture.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        capture_context* context;
        create_context(&context);
        
        capture_device* devices;
        unsigned int devices_length;
        list_devices(context, &devices, &devices_length);
        for (int i = 0; i < devices_length; i++) {
            NSLog(@"%s, %s, %s, %s", devices[i].unique_id, devices[i].manufacturer, devices[i].model, devices[i].name);
            NSLog(@"    auto focus %d, manual focus %d, auto exposure %d, manual exposure %d", devices[i].supportsFocusAuto, devices[i].supportsFocusManual, devices[i].supportsExposureAuto, devices[i].supportsExposureManual);
            for (int j = 0; j < devices[i].formats_length; j++) {
                NSLog(@"        fourcc %u, fps %u, width %u, height %u",
                      devices[i].formats[j].fourcc,
                      devices[i].formats[j].fps,
                      devices[i].formats[j].width,
                      devices[i].formats[j].height);
            }
        }
        
        capture_session* session;
        open_session(&devices[1], &session);
        
        release_context(context);
    }
    return 0;
}
