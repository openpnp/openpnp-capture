/*

    OpenPnp-Capture: a video capture subsystem.

    Created by Niels Moseley on 7/11/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform independent context class to keep track
    of the global state.

*/

#ifndef linux_platformcontext_h
#define linux_platformcontext_h

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
    bool queryFrameSize(int fd, uint32_t index, uint32_t pixelformat, uint32_t *width, uint32_t *height);

    uint32_t findMaxFrameRate(int fd, uint32_t pixelformat, uint32_t width, uint32_t height);

    /** Enumerate V4L capture devices and put their 
        information into the m_devices array 
    */
    virtual bool enumerateDevices();

};

#endif