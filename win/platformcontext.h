/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent context class to keep track
    of the global state.

    Created by Niels Moseley on 7/11/17.
    Copyright (c) 2017 Niels Moseley.

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

#ifndef win_platformcontext_h
#define win_platformcontext_h

#include <vector>
#include <string>
#include <map>
#include <stdint.h>

#pragma comment(lib, "strmiids")

#include "openpnp-capture.h"

#include "platformdeviceinfo.h"
#include "../common/context.h"

/** context base class keeps track of all the platform independent
    objects and information */

class PlatformContext : public Context
{
public:
    /** Create a context for the library.
        Device enumeration is performed in the constructor,
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