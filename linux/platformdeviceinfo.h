#ifndef platformdeviceinfo_h
#define platformdeviceinfo_h

#include <linux/videodev2.h>
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

    std::string     m_devicePath;   ///< unique device path
};

#endif