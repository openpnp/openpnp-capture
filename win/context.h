/*

    OpenPnp-Capture: a video capture subsystem.

    Windows platform code

    Created by Niels Moseley on 7/6/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform/implementation specific structures
    and typedefs.

*/

#ifndef openpnp_context_h
#define openpnp_context_h

#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "strmiids")
//#pragma comment(lib, "Mfplat")
//#pragma comment(lib, "Mf.lib")

#include <windows.h>
#include <dshow.h>
#include <vector>
#include <string>
#include <stdint.h>

#include "openpnp-capture.h"

class Stream;

/** device information struct/object */
struct deviceInfo
{
    deviceInfo()  : m_moniker(0) {}
    ~deviceInfo()
    {
        if (m_moniker != nullptr)
        {
            //FIXME: not sure what to do with
            // IMoniker* here. When I call
            // ->Release(), the program crashes ?
            // even an additional AddRef was
            // applied.. ? Documentation unclear.
        }
    }

    std::string     m_name;         ///< UTF-8 printable name
    std::wstring    m_filterName;   ///< DirectShow internal device name
    std::wstring    m_devicePath;   ///< unique device path
    IMoniker*       m_moniker;      ///< DirectShow object for capture device        
};

/** context class keeps track of all the platform dependent
    objects and information */

class Context
{
public:
    /** Create a context for the library.
        Device enumeration is perform in the constructor,
        so all devices must be present in the system when
        the Context is created or devices will not be found.

        Re-enumeration support is pending.
    */
    Context();
    virtual ~Context();

    /** Get the UTF-8 device name of a device with index/ID id */
    const char* getDeviceName(CapDeviceID id) const;

    /** Return the number of devices found */
    uint32_t    getDeviceCount() const;

    /** Opens a stream to a device with index/ID id and returns the stream ID.
        If an error occurs (device not found), -1 is returned.

        If the stream is succesfully opnened, capturing starts automatically
        until the stream (or its associated context) is closed with closeStream.

        Note: for now, only one stream per device is supported but opening more
              streams might or might not work.
    */
    int32_t openStream(CapDeviceID id);

    /** close the stream to a device */
    void closeStream(int32_t streamID);

protected:
    /** Enumerate DirectShow capture devices and put their 
        information into the m_devices array */
    bool enumerateDevices();

    /** Convert a wide character string to an UTF-8 string */
    std::string wstringToString(const std::wstring &wstr);

    /** Convert a wide charater string to an UTF-8 string */
    std::string wcharPtrToString(const wchar_t *str);
    
    std::vector<deviceInfo> m_devices;
    std::vector<Stream*>    m_streams;
};

#endif