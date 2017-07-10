#ifndef deviceinfo_h
#define deviceinfo_h

#include <windows.h>
#include <Dshow.h>
#include <string>

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

#endif