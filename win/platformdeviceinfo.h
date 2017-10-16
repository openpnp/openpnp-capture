#ifndef win_platformdeviceinfo_h
#define win_platformdeviceinfo_h

#include <windows.h>
#include <Dshow.h>
#include <string>

#include "../common/deviceinfo.h"

/** device information struct/object */
class platformDeviceInfo : public deviceInfo
{
public:
    platformDeviceInfo() : deviceInfo(), m_moniker(0) {}

    ~platformDeviceInfo()
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

    std::wstring    m_filterName;   ///< DirectShow internal device name
    std::wstring    m_devicePath;   ///< unique device path
    IMoniker*       m_moniker;      ///< DirectShow object for capture device
};

#endif