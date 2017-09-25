/*

    OpenPnp-Capture: a video capture subsystem.

    OSX platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley, Jason von Nieda.

    UVC camera controls

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
#define UVC_PROCESSING_UNIT_ID 0x03

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
#define PU_BRIGHTNESS_CONTROL 0x02
#define PU_CONTRAST_CONTROL 0x03
#define PU_GAIN_CONTROL 0x04
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define PA_WHITE_BALANCE_COMPONENT_CONTROL 0x0C
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


UVCCtrl::UVCCtrl(IOUSBInterfaceInterface190 **controller)
    : m_controller(controller)
{
    LOG(LOG_VERBOSE,"[BRIGHTNESS] ");
    reportCapabilities(PU_BRIGHTNESS_CONTROL, UVC_PROCESSING_UNIT_ID);        
}

UVCCtrl::~UVCCtrl()
{
    if (m_controller != nullptr)
    {
        (*m_controller)->USBInterfaceClose(m_controller);
        (*m_controller)->Release(m_controller);
    }
}

IOUSBInterfaceInterface190** UVCCtrl::findDevice(uint16_t vid, uint16_t pid)
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
            result = (*deviceInterface)->GetDeviceVendor(deviceInterface, &vendorID);
            result = (*deviceInterface)->GetDeviceProduct(deviceInterface, &productID);

            if ((vendorID == vid) && (productID == pid))
            {
                getProcessingUnitID(deviceInterface);
                return createControlInterface(deviceInterface);
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
        //LOG(LOG_VERBOSE,"  type      = %08X\n", configDesc->type);
        LOG(LOG_VERBOSE,"  totalLen  = %08X\n", configDesc->wTotalLength);
        LOG(LOG_VERBOSE,"  interfaces = %08X\n", configDesc->bNumInterfaces);

        FILE *fout = fopen("usbdump.txt","wb");
        fwrite(configDesc, 1, configDesc->wTotalLength, fout);
        fclose(fout);
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

    kern_return_t kr = (*m_controller)->USBInterfaceOpen(m_controller);
    if (kr != kIOReturnSuccess)
    {
        LOG(LOG_ERR, "sendControlRequest USBInterfaceOpen failed!\n");
        return false;
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
                LOG(LOG_ERR,"sendControlRequest: Pipe has stalled, error needs to be cleared\n");
                break;
            case kIOUSBInterfaceNotFound:
                LOG(LOG_ERR,"sendControlRequest: Pipe has stalled, error needs to be cleared\n");
                break;
            default:
                LOG(LOG_ERR, "sendControlRequest ControlRequest failed (KR=sys:sub:code) = %02Xh:%03Xh:%04Xh)!\n", 
                    err_get_system(kr), err_get_sub(kr), err_get_code(kr));
                break; 
        }

        kr = (*m_controller)->USBInterfaceClose(m_controller);
        return false;
    }

    kr = (*m_controller)->USBInterfaceClose(m_controller);
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
}

bool UVCCtrl::getProperty(uint32_t propID, int32_t *value)
{
    if (m_controller == nullptr)
    {
        return false;
    }

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
        return setData(PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, UVC_PROCESSING_UNIT_ID, 1, value);
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
        if (getData(PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, UVC_PROCESSING_UNIT_ID, 1, &value))
        {
            LOG(LOG_VERBOSE,"PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL returned %08Xh\n", value & 0xFF);
            value &= 0xFF; // make 8-bit            
            *enabled = (value==1) ? true : false; 
            return true;
        }
        return false;     
    default:
        return false;
    }

    return false;   // we should never get here..
}

bool UVCCtrl::getPropertyLimits(uint32_t propID, int32_t *emin, int32_t *emax)
{
    if (m_controller == nullptr)
    {
        return false;
    }

    switch(propID)
    {
    case CAPPROPID_EXPOSURE:
        LOG(LOG_INFO, "UVCCtrl::getPropertyLimits (exposure)\n");
        if (!getMinData(CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 4, emin))
        {
            return false;
        }
        if (!getMaxData(CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 4, emax))
        {
            return false;
        }
        return true;
    case CAPPROPID_FOCUS:
        LOG(LOG_INFO, "UVCCtrl::getPropertyLimits (focus)\n");
        if (!getMinData(CT_FOCUS_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 2, emin))
        {
            return false;
        }
        if (!getMaxData(CT_FOCUS_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 2, emax))
        {
            return false;
        }
        // convert from 16-bit to 32-bit.
        *emin = (int16_t)(*emin & 0xFFFF);
        *emax = (int16_t)(*emax & 0xFFFF);        
        return true;
    case CAPPROPID_ZOOM:
        LOG(LOG_INFO, "UVCCtrl::getPropertyLimits (zoom)\n");
        if (!getMinData(CT_ZOOM_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 2, emin))
        {
            return false;
        }
        if (!getMaxData(CT_ZOOM_ABSOLUTE_CONTROL, UVC_INPUT_TERMINAL_ID, 2, emax))
        {
            return false;
        }
        // convert from 16-bit to 32-bit.
        *emin = (int16_t)(*emin & 0xFFFF);
        *emax = (int16_t)(*emax & 0xFFFF);        
        return true; 
    case CAPPROPID_WHITEBALANCE:
        LOG(LOG_INFO, "UVCCtrl::getPropertyLimits (white balance)\n");
        if (!getMinData(PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_PROCESSING_UNIT_ID, 2, emin))
        {
            reportCapabilities(PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_PROCESSING_UNIT_ID);
            return false;
        }
        if (!getMaxData(PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_PROCESSING_UNIT_ID, 2, emax))
        {
            reportCapabilities(PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_PROCESSING_UNIT_ID);
            return false;
        }     
        // convert from 16-bit to 32-bit.
        *emin = (int16_t)(*emin & 0xFFFF);
        *emax = (int16_t)(*emax & 0xFFFF); 
        return true;
    case CAPPROPID_GAIN:
        LOG(LOG_INFO, "UVCCtrl::getPropertyLimits (gain)\n");
        if (!getMinData(PU_GAIN_CONTROL, UVC_PROCESSING_UNIT_ID, 2, emin))
        {
            reportCapabilities(PU_GAIN_CONTROL,UVC_PROCESSING_UNIT_ID);
            return false;
        }
        if (!getMaxData(PU_GAIN_CONTROL, UVC_PROCESSING_UNIT_ID, 2, emax))
        {
            reportCapabilities(PU_GAIN_CONTROL,UVC_PROCESSING_UNIT_ID);
            return false;
        }
        // convert from 16-bit to 32-bit.
        *emin = (int16_t)(*emin & 0xFFFF);
        *emax = (int16_t)(*emax & 0xFFFF);        
        return true;
    default:
        LOG(LOG_INFO, "UVCCtrl::getPropertyLimits (unsupported property %d)\n", propID);
        return false;
    }

    return false;   
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