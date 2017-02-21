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
        NSLog(@"start");
        capture_context* context;
        create_context(&context);
        
//        capture_device* devices;
//        unsigned int devices_length;
//        list_devices(context, &devices, &devices_length);
//        list_devices(context, &devices, &devices_length);
//        list_devices(context, &devices, &devices_length);
//        list_devices(context, &devices, &devices_length);
//        for (int i = 0; i < devices_length; i++) {
//            NSLog(@"%s, %s, %s, %s\n", devices[i].unique_id, devices[i].manufacturer, devices[i].model, devices[i].name);
//        }
        
        release_context(context);
        NSLog(@"stop");
    }
    return 0;
}
