#ifndef platformdeviceinfo_h
#define platformdeviceinfo_h

#include <string>

#include "../common/deviceinfo.h"

/** device information struct/object */
class platformDeviceInfo : public deviceInfo
{
public:
    platformDeviceInfo() : deviceInfo() 
    {

    }

    virtual ~platformDeviceInfo()
    {

    }
    
    std::string m_uniqueId;
};

#endif
