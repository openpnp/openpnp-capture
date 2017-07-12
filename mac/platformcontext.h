#ifndef openpnp_macplatformcontext_h
#define openpnp_macplatformcontext_h

#include "openpnp-capture.h"

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
    /** Enumerate V4L capture devices and put their 
        information into the m_devices array 
        
        Implement this function in a platform-dependent
        derived class.
    */
    virtual bool enumerateDevices();

};

#endif
