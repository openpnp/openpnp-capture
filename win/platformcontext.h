/*

    OpenPnp-Capture: a video capture subsystem.

    Created by Niels Moseley on 7/11/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform independent context class to keep track
    of the global state.

*/

#ifndef openpnp_win32platformcontext_h
#define openpnp_win32platformcontext_h

#define _CRT_SECURE_NO_WARNINGS


#include <vector>
#include <string>
#include <map>
#include <stdint.h>

#include "openpnp-capture.h"

#pragma comment(lib, "strmiids")
#include "platformdeviceinfo.h"
#include "../common/context.h"

/** context base class keeps track of all the platform independent
    objects and information */

class PlatformContext : public Context
{
public:
    /** Create a context for the library.
        Device enumeration is perform in the constructor,
        so all devices must be present in the system when
        the Context is created or devices will not be found.

        Re-enumeration support is pending.
    */
    PlatformContext();
    virtual ~PlatformContext();

protected:

    /** retrieve all the frame information and write it to the platformDeviceInfo object */
    bool enumerateFrameInfo(IMoniker *moniker, platformDeviceInfo *info);

    /** Enumerate DirectShow capture devices and put their 
        information into the m_devices array 
        
        Implement this function in a platform-dependent
        derived class.
    */
    virtual bool enumerateDevices();

    /** Convert a wide character string to an UTF-8 string 
        
        Implement this function in a platform-dependent
        derived class.    
    */
    virtual std::string wstringToString(const std::wstring &wstr);

    /** Convert a wide charater string to an UTF-8 string
        
        Implement this function in a platform-dependent
        derived class.    
    */
    virtual std::string wcharPtrToString(const wchar_t *str);

};

#endif