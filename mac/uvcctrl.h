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

    /** create a UVC controller for a USB camera based on its 
        USB vendor ID (vid), USB product ID (pid) and the
        bus location.

        If the bus location is unknown, set it to zero. In this case,
        a control interface to the first matching VID/PID device is
        returned.
    */
    static UVCCtrl* create(uint16_t vid, uint16_t pid, uint32_t location)
    {
        IOUSBDeviceInterface** dev = findDevice(vid, pid, location);
        if (dev != nullptr)
        {
            uint32_t unitID = getProcessingUnitID(dev);
            IOUSBInterfaceInterface190** iface = createControlInterface(dev);
            if (iface != nullptr)
            {
                return new UVCCtrl(iface, unitID);
            }
            (*dev)->Release(dev);
        }
        return nullptr;
    }

    bool setProperty(uint32_t propID, int32_t value);
    bool getProperty(uint32_t propID, int32_t *value);
    bool setAutoProperty(uint32_t propID, bool enabled);
    bool getAutoProperty(uint32_t propID, bool *enabled);
    bool getPropertyLimits(uint32_t propID, int32_t *emin, int32_t *emax);

protected:
    UVCCtrl(IOUSBInterfaceInterface190 **controller, uint32_t processingUnitID);

    static IOUSBDeviceInterface** findDevice(uint16_t vid, uint16_t pid, uint32_t location);
    static IOUSBInterfaceInterface190** createControlInterface(IOUSBDeviceInterface** deviceInterface);
    static uint32_t getProcessingUnitID(IOUSBDeviceInterface**);

    bool sendControlRequest(IOUSBDevRequest req);
    bool setData(uint32_t selector, uint32_t unit, uint32_t length, int32_t data);
    bool getData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);
    bool getMaxData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);
    bool getMinData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);
    bool getInfo(uint32_t selector, uint32_t unit, uint32_t *data);
    bool getDefault(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data);

    void reportCapabilities(uint32_t selector, uint32_t unit);

    uint32_t m_pud; // processing unit ID
    IOUSBInterfaceInterface190** m_controller;
};

#endif