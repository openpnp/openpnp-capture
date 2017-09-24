/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley, Jason von Nieda.

    UVC camera controls

*/

#ifndef uvcctrl_h
#define uvcctrl_h

#import <CoreFoundation/CoreFoundation.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "../include/openpnp-capture.h"

class UVCCtrl
{
public:
    virtual ~UVCCtrl();

    /** create a UVC controller for a USB camera */
    static UVCCtrl* create(uint16_t vid, uint16_t pid)
    {
        IOUSBInterfaceInterface190** iface = findDevice(vid, pid);
        if (iface == nullptr)
        {
            return nullptr;
        }
        else
        {
            return new UVCCtrl(iface);
        }
    }

    bool setProperty(uint32_t propID, int32_t value);
    bool getProperty(uint32_t propID, int32_t *value);
    bool setAutoProperty(uint32_t propID, bool enabled);
    bool getAutoProperty(uint32_t propID, bool *enabled);
    bool getPropertyLimits(uint32_t propID, int32_t *emin, int32_t *emax);

protected:
    UVCCtrl(IOUSBInterfaceInterface190 **controller)
        : m_controller(controller)
    {
    }

    static IOUSBInterfaceInterface190** findDevice(uint16_t vid, uint16_t pid);
    static IOUSBInterfaceInterface190** createControlInterface(IOUSBDeviceInterface** deviceInterface);

    bool sendControlRequest(IOUSBDevRequest req);
    bool setData(uint32_t selector, uint32_t unit, uint32_t length, int32_t data);
    bool getData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);
    bool getMaxData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);
    bool getMinData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);
    bool getInfo(uint32_t selector, uint32_t unit, uint32_t *data);

    IOUSBInterfaceInterface190** m_controller;
};

#endif