/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform code
    UVC camera controls

    Created by Niels Moseley on 7/6/17.
    Copyright (c) 2017 Jason von Nieda, Niels Moseley.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

#include <IOKit/usb/USB.h>
#include "uvcctrl.h"
#include "../common/logging.h"

//
// constants defined in the UVC USB specfication
// which you can get here: http://www.usb.org/developers/docs/devclass_docs/USB_Video_Class_1_5.zip
// 
// FIXME: we should get the processing unit ID from the USB device 
// descriptors
//

#define UVC_INPUT_TERMINAL_ID 0x01

// Camera termainal control selectors
#define CT_AE_MODE_CONTROL 0x02
#define CT_AE_PRIORITY_CONTROL 0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define CT_FOCUS_ABSOLUTE_CONTROL 0x06
#define CT_FOCUS_RELATIVE_CONTROL 0x07
#define CT_FOCUS_AUTO_CONTROL 0x08
#define CT_ZOOM_ABSOLUTE_CONTROL 0x0B
#define CT_ZOOM_RELATIVE_CONTROL 0x0C

// Processing unit control selectors
#define PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define PU_BRIGHTNESS_CONTROL 0x02
#define PU_CONTRAST_CONTROL 0x03
#define PU_GAIN_CONTROL 0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define PU_HUE_CONTROL 0x06
#define PU_SATURATION_CONTROL 0x07
#define PU_SHARPNESS_CONTROL 0x08
#define PU_GAMMA_CONTROL 0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define PU_HUE_AUTO_CONTROL 0x10
#define PU_CONTRAST_AUTO_CONTROL 0x13

// Video control interface selectors
#define VC_REQUEST_ERROR_CODE_CONTROL 0x02

#define UVC_CONTROL_INTERFACE_CLASS 14
#define UVC_CONTROL_INTERFACE_SUBCLASS 1

#define UVC_SET_CUR	0x01
#define UVC_GET_CUR	0x81
#define UVC_GET_MIN	0x82
#define UVC_GET_MAX	0x83
#define UVC_GET_RES 0x84
#define UVC_GET_INFO 0x86
#define UVC_GET_DEF 0x87

// USB descriptor for UVC processing unit
struct ProcessingUnitDescriptor
{
    uint8_t bLength;
    uint8_t bDescriptorType;        // CS_INTERFACE 0x24
    uint8_t bDescriptorSubtype;     // VC_PROCESSING_UNIT 0x05
    uint8_t bUnitID;
};

struct propertyInfo_t
{
    uint32_t    selector;   // selector ID
    uint32_t    unit;       // unit (==0 for INPUT TERMINA:, ==1 for PROCESSING UNIT)
    uint32_t    length;     // length (bytes)
};

/** The order of the propertyInfo structure must
    be the same as the PROPID numbers in the
    openpnp-capture.h header */
const propertyInfo_t propertyInfo[] =
{
    {0,0,0},
    {CT_EXPOSURE_TIME_ABSOLUTE_CONTROL   , 0, 4},
    {CT_FOCUS_ABSOLUTE_CONTROL           , 0, 2},
    {CT_ZOOM_ABSOLUTE_CONTROL            , 0, 2},
    {PU_WHITE_BALANCE_TEMPERATURE_CONTROL, 1, 2},
    {PU_GAIN_CONTROL                     , 1, 2},
    {PU_BRIGHTNESS_CONTROL               , 1, 2},
    {PU_CONTRAST_CONTROL                 , 1, 2},
    {PU_SATURATION_CONTROL               , 1, 2},
    {PU_GAMMA_CONTROL                    , 1, 2},
    {PU_HUE_CONTROL                      , 1, 2},
    {PU_SHARPNESS_CONTROL                , 1, 2},
    {PU_BACKLIGHT_COMPENSATION_CONTROL   , 1, 2},
    {PU_POWER_LINE_FREQUENCY_CONTROL     , 1, 1}
};


UVCCtrl::UVCCtrl(IOUSBInterfaceInterface190 **controller, uint32_t processingUnitID)
    : m_pud(processingUnitID),
    m_controller(controller)
{
    //LOG(LOG_VERBOSE,"[BRIGHTNESS] ");
    //reportCapabilities(PU_BRIGHTNESS_CONTROL, UVC_PROCESSING_UNIT_ID);        
}

UVCCtrl::~UVCCtrl()
{
    if (m_controller != nullptr)
    {
        (*m_controller)->USBInterfaceClose(m_controller);
        (*m_controller)->Release(m_controller);
    }
}

IOUSBDeviceInterface** UVCCtrl::findDevice(uint16_t vid, uint16_t pid, uint32_t location)
{
    LOG(LOG_DEBUG, "UVCCtrl::findDevice() called\n");

    CFMutableDictionaryRef dict = IOServiceMatching(kIOUSBDeviceClassName);

    io_iterator_t serviceIterator;
    IOServiceGetMatchingServices(kIOMasterPortDefault, dict, &serviceIterator);

    io_service_t device;
    while((device = IOIteratorNext(serviceIterator)) != 0)
    {
        IOUSBDeviceInterface **deviceInterface = nullptr;
        IOCFPlugInInterface  **plugInInterface = nullptr;
        SInt32 score;

        kern_return_t result = IOCreatePlugInInterfaceForService(
            device, kIOUSBDeviceUserClientTypeID,
            kIOCFPlugInInterfaceID, &plugInInterface, &score);

        if ((result != kIOReturnSuccess) || (plugInInterface == nullptr))
        {
            LOG(LOG_DEBUG, "UVCCtrl::findDevice() Camera control error %d\n", result);
            continue;
        }
        else
        {
            HRESULT hr = (*plugInInterface)->QueryInterface(plugInInterface,
                CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                (LPVOID*)&deviceInterface);

            if (hr || (deviceInterface == nullptr))
            {
                (*plugInInterface)->Release(plugInInterface);
                //FIXME: log error
                continue;
            }

            uint16_t vendorID, productID;
            uint32_t locationID;
            result = (*deviceInterface)->GetDeviceVendor(deviceInterface, &vendorID);
            result = (*deviceInterface)->GetDeviceProduct(deviceInterface, &productID);
            result = (*deviceInterface)->GetLocationID(deviceInterface, &locationID);

            // if 'location' is zero, we won't match on location
            // to achieve this, we simply set locationID to zero.
            if (location == 0)
            {
                locationID = 0;
            }

            if ((vendorID == vid) && (productID == pid) && (locationID == location))
            {
                (*plugInInterface)->Release(plugInInterface);
                return deviceInterface;
            }

            (*plugInInterface)->Release(plugInInterface);
        }
    }

    return NULL;
}

uint32_t UVCCtrl::getProcessingUnitID(IOUSBDeviceInterface** dev)
{
    // find the ID of the processing unit in the USB device descriptor
    //Get the configuration descriptor for index 0

    IOReturn                        kr;
    IOUSBConfigurationDescriptorPtr configDesc;

    kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &configDesc);
    if (kr)
    {
        return 0; // cannot find the ID
    }
    else
    {
        LOG(LOG_VERBOSE,"USB descriptor:\n");
        LOG(LOG_VERBOSE,"  length    = %08X\n", configDesc->bLength);
        LOG(LOG_VERBOSE,"  type      = %08X\n", configDesc->bDescriptorType);
        LOG(LOG_VERBOSE,"  totalLen  = %08X\n", configDesc->wTotalLength);
        LOG(LOG_VERBOSE,"  interfaces = %08X\n", configDesc->bNumInterfaces);

        //FILE *fout = fopen("usbdump.txt","wb");
        //fwrite(configDesc, 1, configDesc->wTotalLength, fout);
        //fclose(fout);

        uint32_t idx  = 0;
        uint8_t  *ptr = (uint8_t*)configDesc;
        
        // Search for VIDEO/CONTROL interface descriptor
        // Class=14, Subclass=1, Protocol=0
        // and find the processing unit, if available..
        // DescriptorType 0x24, DescriptorSubType 0x5

        IOUSBInterfaceDescriptor *iface = NULL;
        ProcessingUnitDescriptor *pud   = NULL;
        bool inVideoControlInterfaceDescriptor = false;
        while(idx < configDesc->wTotalLength)
        {
            IOUSBDescriptorHeader *hdr = (IOUSBDescriptorHeader *)&ptr[idx];
            switch(hdr->bDescriptorType)
            {
                case 0x05:  // Endpoint descriptor ID
                    LOG(LOG_VERBOSE,"Endpoint\n");
                    break;
                case 0x02:  // Configuration descriptor ID
                    LOG(LOG_VERBOSE,"Configuration\n");
                    break;
                case 0x04: // Interface descriptor ID
                    LOG(LOG_VERBOSE,"Interface");
                    iface = (IOUSBInterfaceDescriptor*)&ptr[idx];
                    if ((iface->bInterfaceClass == 14) && 
                        (iface->bInterfaceSubClass == 1) &&
                        (iface->bInterfaceProtocol == 0))
                    {
                        inVideoControlInterfaceDescriptor = true;
                        LOG(LOG_VERBOSE," VIDEO/CONTROL\n");
                    }
                    else
                    {
                        inVideoControlInterfaceDescriptor = false;
                        LOG(LOG_VERBOSE,"\n");
                    }
                    break;
                case 0x24: // class-specific ID
                    pud = (ProcessingUnitDescriptor*)&ptr[idx];
                    if (inVideoControlInterfaceDescriptor)
                    {
                        if (pud->bDescriptorSubtype == 0x05)
                        {
                            LOG(LOG_VERBOSE,"Processing Unit ID: %d\n", 
                                pud->bUnitID);
                            return pud->bUnitID;
                        }
                    }
                    //LOG(LOG_VERBOSE,"\n");
                    break;
                default:
                    //LOG(LOG_VERBOSE,"?\n");
                    break;
            }
            idx += hdr->bLength; // skip to next..
        }
    }
    return 0;
}

IOUSBInterfaceInterface190** UVCCtrl::createControlInterface(IOUSBDeviceInterface** deviceInterface)
{
    IOUSBInterfaceInterface190 **controlInterface;

    io_iterator_t interfaceIterator;
    IOUSBFindInterfaceRequest interfaceRequest;
    interfaceRequest.bInterfaceClass = UVC_CONTROL_INTERFACE_CLASS;
    interfaceRequest.bInterfaceSubClass = UVC_CONTROL_INTERFACE_SUBCLASS;
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bAlternateSetting  = kIOUSBFindInterfaceDontCare;

    IOReturn result = (*deviceInterface)->CreateInterfaceIterator(deviceInterface,
        &interfaceRequest, &interfaceIterator);

    if (result != kIOReturnSuccess)
    {
        return NULL;
    }

    io_service_t usbInterface;
    HRESULT hr;

    if ((usbInterface = IOIteratorNext(interfaceIterator)) != 0)
    {
        IOCFPlugInInterface **plugInInterface = NULL;
        SInt32 score;
        kern_return_t kr = IOCreatePlugInInterfaceForService(usbInterface,
            kIOUSBInterfaceUserClientTypeID, 
            kIOCFPlugInInterfaceID, 
            &plugInInterface,
            &score);

        kr = IOObjectRelease(usbInterface);
        if ((kr != kIOReturnSuccess) || !plugInInterface)
        {
            LOG(LOG_ERR, "UVCCtrl::createControlInterface cannot create plug-in %08X\n",
                kr);
            return NULL;
        }

        hr = (*plugInInterface)->QueryInterface(plugInInterface,
            CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
            (LPVOID*) &controlInterface);

        (*plugInInterface)->Release(plugInInterface);

        if (hr || !controlInterface)
        {
            LOG(LOG_ERR,"UVCCtrl::createControlInterface: cannot create device interface %08X\n",
                result);
                return NULL;
        }
        LOG(LOG_DEBUG, "UVCCtrl::createControlInterface: created control interface\n");

        return controlInterface;
    }
    return NULL;
}

bool UVCCtrl::sendControlRequest(IOUSBDevRequest req)
{
    if (m_controller == nullptr)
    {
        LOG(LOG_ERR,"UVCCtrl::sendControlRequest: control interface is NULL\n");
        return false;
    }

    kern_return_t kr;
    if (@available(macOS 12.0, *))
    {
        // macOS 12 doesn't like if we're trying to open USB interface here...
    }
    else
    {
        kr = (*m_controller)->USBInterfaceOpen(m_controller);
        if (kr != kIOReturnSuccess)
        {
            LOG(LOG_ERR, "sendControlRequest USBInterfaceOpen failed!\n");
            return false;
        }
    }

    kr = (*m_controller)->ControlRequest(m_controller, 0, &req);
    if (kr != kIOReturnSuccess)
    {
        // IOKIT error code
        #define err_get_system(err) (((err)>>26)&0x3f) 
        #define err_get_sub(err) (((err)>>14)&0xfff) 
        #define err_get_code(err) ((err)&0x3fff)
        
        uint32_t code = err_get_code(kr);
        uint32_t sys  = err_get_system(kr);
        uint32_t sub  = err_get_sub(kr);

        switch(kr)
        {
            case kIOUSBUnknownPipeErr:
                LOG(LOG_ERR,"sendControlRequest: Pipe ref not recognised\n");
                break;
            case kIOUSBTooManyPipesErr:
                LOG(LOG_ERR,"sendControlRequest: Too many pipes\n");
                break;
            case kIOUSBEndpointNotFound:
                LOG(LOG_ERR,"sendControlRequest: Endpoint not found\n");
                break;
            case kIOUSBConfigNotFound:
                LOG(LOG_ERR,"sendControlRequest: USB configuration not found\n");
                break;
            case kIOUSBPipeStalled:
                //Note: we don't report this as an error as this happens when
                //      an unsupported or locked property is set.            
                LOG(LOG_VERBOSE,"sendControlRequest: Pipe has stalled, error needs to be cleared\n");
                break;
            case kIOUSBInterfaceNotFound:
                LOG(LOG_ERR,"sendControlRequest: USB control interface not found\n");
                break;
            default:
                LOG(LOG_ERR, "sendControlRequest ControlRequest failed (KR=sys:sub:code) = %02Xh:%03Xh:%04Xh)!\n", 
                    err_get_system(kr), err_get_sub(kr), err_get_code(kr));
                break; 
        }

        if (@available(macOS 12.0, *))
        {
            // macOS 12 doesn't like if we're trying to close USB interface here...
        }
        else
        {
            kr = (*m_controller)->USBInterfaceClose(m_controller);
            if (kr != kIOReturnSuccess) {
                LOG(LOG_ERR, "sendControlRequest USBInterfaceClose failed!\n");
            }
        }

        return false;
    }

    if (@available(macOS 12.0, *))
    {
        // macOS 12 doesn't like if we're trying to close USB interface here either...
    }
    else
    {
        kr = (*m_controller)->USBInterfaceClose(m_controller);
        if (kr != kIOReturnSuccess)
        {
            LOG(LOG_ERR, "sendControlRequest USBInterfaceClose failed!\n");
        }
    }

    return true;
}

bool UVCCtrl::setData(uint32_t selector, uint32_t unit, uint32_t length, int32_t data)
{
    IOUSBDevRequest req;
    req.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
    req.bRequest = UVC_SET_CUR;
    req.wValue   = (selector << 8);
    req.wIndex   = (unit << 8);
    req.wLength  = length;
    req.wLenDone = 0;
    req.pData = &data;
    return sendControlRequest(req);
}

bool UVCCtrl::getData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data)
{
    IOUSBDevRequest req;
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    req.bRequest = UVC_GET_CUR;
    req.wValue   = (selector << 8);
    req.wIndex   = (unit << 8);
    req.wLength  = length;
    req.wLenDone = 0;
    req.pData = data;
    return sendControlRequest(req);
}

bool UVCCtrl::getMaxData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data)
{
    IOUSBDevRequest req;
    *data = 0;
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    req.bRequest = UVC_GET_MAX;
    req.wValue   = (selector << 8);
    req.wIndex   = (unit << 8);
    req.wLength  = length;
    req.wLenDone = 0;
    req.pData = data;
    return sendControlRequest(req);
}

bool UVCCtrl::getMinData(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data)
{
    IOUSBDevRequest req;
    *data = 0;
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    req.bRequest = UVC_GET_MIN;
    req.wValue   = (selector << 8);
    req.wIndex   = (unit << 8);
    req.wLength  = length;
    req.wLenDone = 0;
    req.pData = data;
    return sendControlRequest(req);
}

bool UVCCtrl::getDefault(uint32_t selector, uint32_t unit, uint32_t length, int32_t *data)
{
    IOUSBDevRequest req;
    *data = 0;
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    req.bRequest = UVC_GET_DEF;
    req.wValue   = (selector << 8);
    req.wIndex   = (unit << 8);
    req.wLength  = length;
    req.wLenDone = 0;
    req.pData = data;
    return sendControlRequest(req);
}

bool UVCCtrl::getInfo(uint32_t selector, uint32_t unit, uint32_t *data)
{
    IOUSBDevRequest req;
    *data = 0;
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    req.bRequest = UVC_GET_INFO;
    req.wValue   = (selector << 8);
    req.wIndex   = (unit << 8);
    req.wLength  = 1;
    req.wLenDone = 0;
    req.pData = data;
    return sendControlRequest(req);    
}

bool UVCCtrl::setProperty(uint32_t propID, int32_t value)
{
    if (m_controller == nullptr)
    {
        return false;
    }

    bool ok = false;
    if (propID < CAPPROPID_LAST)
    {
        uint32_t unit = (propertyInfo[propID].unit == 0) ? UVC_INPUT_TERMINAL_ID : m_pud;
        ok = setData(propertyInfo[propID].selector, unit, 
            propertyInfo[propID].length, value);
    }
    return ok;

#if 0
    switch(propID)
    {
    case CAPPROPID_EXPOSURE:
        LOG(LOG_VERBOSE, "UVCCtrl::setProperty (exposure) %08X\n", value);
        return setData(CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 4, value);
    case CAPPROPID_FOCUS:
        return false; // FIXME: not supported yet
    case CAPPROPID_ZOOM:
        LOG(LOG_VERBOSE, "UVCCtrl::setProperty (zoom) %08X\n", value);
        return setData(CT_ZOOM_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 4, value);
    case CAPPROPID_WHITEBALANCE:
        LOG(LOG_VERBOSE, "UVCCtrl::setProperty (white balance) %08X\n", value);
        return setData(PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_PROCESSING_UNIT_ID, 2, value);
    case CAPPROPID_GAIN:
        LOG(LOG_VERBOSE, "UVCCtrl::setProperty (gain) %08X\n", value);
        return setData(PU_GAIN_CONTROL, UVC_PROCESSING_UNIT_ID, 2, value);
    default:
        return false;
    }

    return false;   // we should never get here..    
#endif

}

bool UVCCtrl::getProperty(uint32_t propID, int32_t *value)
{
    if (m_controller == nullptr)
    {
        return false;
    }

    bool ok = false;
    if (propID < CAPPROPID_LAST)
    {
        uint32_t unit = (propertyInfo[propID].unit == 0) ? UVC_INPUT_TERMINAL_ID : m_pud;
        ok = getData(propertyInfo[propID].selector, unit, 
            propertyInfo[propID].length, value);

        switch(propertyInfo[propID].length)
        {
            case 2:
                *value = static_cast<int16_t>(*value);
                break;
            case 1:
                *value = static_cast<int8_t>(*value);
                break;
            default:
                break;
        }
    }
    return ok;

#if 0
    *value = 0;
    uint32_t info;
    switch(propID)
    {
    case CAPPROPID_EXPOSURE:
        LOG(LOG_VERBOSE, "UVCCtrl::getProperty (exposure)\n");
        return getData(CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 4, value);
    case CAPPROPID_FOCUS:
        LOG(LOG_VERBOSE, "UVCCtrl::getProperty (focus)\n");
        return false; // FIXME: not supported yet
    case CAPPROPID_ZOOM:
        LOG(LOG_VERBOSE, "UVCCtrl::getProperty (zoom)\n");
        return getData(CT_ZOOM_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 4, value);
    case CAPPROPID_WHITEBALANCE:
        LOG(LOG_VERBOSE, "UVCCtrl::getProperty (white balance)\n");  
        return getData(PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_PROCESSING_UNIT_ID, 2, value);       
    case CAPPROPID_GAIN:
        LOG(LOG_VERBOSE, "UVCCtrl::getProperty (gain)\n");
        return getData(PU_GAIN_CONTROL, UVC_PROCESSING_UNIT_ID, 2, value);    
    default:
        return false;
    }

    return false;   // we should never get here..
#endif
}

bool UVCCtrl::setAutoProperty(uint32_t propID, bool enabled)
{
    if (m_controller == nullptr)
    {
        return false;
    }

    int32_t value = enabled ? 1 : 0;
    switch(propID)
    {
    case CAPPROPID_EXPOSURE:
        LOG(LOG_VERBOSE, "UVCCtrl::setAutoProperty (exposure %s)\n", enabled ? "ON" : "OFF");
        return setData(CT_AE_MODE_CONTROL, UVC_INPUT_TERMINAL_ID, 1, enabled ? 0x8 : 0x1);
    case CAPPROPID_WHITEBALANCE:
        LOG(LOG_VERBOSE, "UVCCtrl::setAutoProperty (white balance %s)\n", enabled ? "ON" : "OFF");
        return setData(PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, m_pud, 1, value);
    case CAPPROPID_FOCUS:
        LOG(LOG_VERBOSE, "UVCCtrl::setAutoProperty (focus %s)\n", enabled ? "ON" : "OFF");
        return setData(CT_FOCUS_AUTO_CONTROL, UVC_INPUT_TERMINAL_ID, 1, value);
    default:
        return false;
    }

    return false;   // we should never get here..
}

bool UVCCtrl::getAutoProperty(uint32_t propID, bool *enabled)
{
    if (m_controller == nullptr)
    {
        return false;
    }

    int32_t value;
    switch(propID)
    {
    case CAPPROPID_EXPOSURE:
        LOG(LOG_VERBOSE, "UVCCtrl::getAutoProperty exposure\n");
        if (getData(CT_AE_MODE_CONTROL, UVC_INPUT_TERMINAL_ID, 1, &value))
        {
            LOG(LOG_VERBOSE,"CT_AE_MODE_CONTROL returned %08Xh\n", value & 0xFF);
            //
            // value = 1 -> manual mode
            //         2 -> auto mode (I haven't seen this in the wild)
            //         4 -> shutter priority mode (haven't seen this)
            //         8 -> aperature prioritry mode (seen this used)

            value &= 0xFF; // make 8-bit
            *enabled = (value==1) ? false : true;
            return true;
        }
        return false;
    case CAPPROPID_WHITEBALANCE:
        LOG(LOG_VERBOSE, "UVCCtrl::getAutoProperty white balance\n");
        if (getData(PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, m_pud, 1, &value))
        {
            LOG(LOG_VERBOSE,"PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL returned %08Xh\n", value & 0xFF);
            value &= 0xFF; // make 8-bit            
            *enabled = (value==1) ? true : false; 
            return true;
        }
        return false;
    case CAPPROPID_FOCUS:
        LOG(LOG_VERBOSE, "UVCCtrl::getAutoProperty focus\n");
        if (getData(CT_FOCUS_AUTO_CONTROL, UVC_INPUT_TERMINAL_ID, 1, &value))
        {
            LOG(LOG_VERBOSE,"CT_FOCUS_AUTO_CONTROL returned %08Xh\n", value & 0xFF);
            value &= 0xFF; // make 8-bit
            *enabled = (value==1) ? true : false;
            return true;
        }
    default:
        return false;
    }

    return false;   // we should never get here..
}

bool UVCCtrl::getPropertyLimits(uint32_t propID, int32_t *emin, int32_t *emax, int32_t *dValue)
{
    if (m_controller == nullptr)
    {
        return false;
    }

    bool ok = true;
    if (propID < CAPPROPID_LAST)
    {
        // get the min data
        uint32_t unit = (propertyInfo[propID].unit == 0) ? UVC_INPUT_TERMINAL_ID : m_pud;
        if (!getMinData(propertyInfo[propID].selector, unit, 
            propertyInfo[propID].length, emin))
        {
            LOG(LOG_VERBOSE, "getMinData failed\n");
            ok = false;
        }

        // get the max data
        if (!getMaxData(propertyInfo[propID].selector, unit, 
            propertyInfo[propID].length, emax))
        {
            LOG(LOG_VERBOSE, "getMaxData failed\n");
            ok = false;
        }

        // read the default value
        if (!getDefault(propertyInfo[propID].selector, unit,
            propertyInfo[propID].length, dValue))
        {
            LOG(LOG_VERBOSE, "getDefault failed\n");
            ok = false;
        }

        switch(propertyInfo[propID].length)
        {
            case 2:
                *emin = static_cast<int16_t>(*emin);
                *emax = static_cast<int16_t>(*emax);
                *dValue = static_cast<int16_t>(*dValue);
                break;
            case 1:
                *emin = static_cast<int8_t>(*emin);
                *emax = static_cast<int8_t>(*emax);
                *dValue = static_cast<int8_t>(*dValue);
                break;
            default:
                break;
        }
    }
    else
    {
        LOG(LOG_ERR, "UVCCtrl::getPropertyLimits property ID out of bounds.\n");
        ok = false;
    }
    return ok;
}

void UVCCtrl::reportCapabilities(uint32_t selector, uint32_t unit)
{
    uint32_t info;
    LOG(LOG_VERBOSE,"CAPS: ");
    getInfo(selector, unit, &info);
    if (info & 0x01)
    {
        LOG(LOG_VERBOSE,"GET ");
    }
    if (info & 0x02)
    {
        LOG(LOG_VERBOSE,"SET ");
    }
    if (info & 0x04)
    {
        LOG(LOG_VERBOSE,"DISABLED ");
    }
    if (info & 0x08)
    {
        LOG(LOG_VERBOSE,"AUTO-UPD ");
    }
    if (info & 0x10)
    {
        LOG(LOG_VERBOSE,"ASYNC ");
    }
    if (info & 0x20)
    {
        LOG(LOG_VERBOSE,"DISCOMMIT");
    }
    LOG(LOG_VERBOSE,"\n");
}